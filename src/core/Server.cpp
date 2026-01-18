#include "core/Server.hpp"
#include "cgi/CGIHandler.hpp"
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @file Server.cpp
 * @brief Main server class - event loop and connection management
 *
 * This is the core orchestrator of the web server. It manages:
 * - Multiple listening sockets (one per unique port)
 * - Client connections via non-blocking poll() event loop
 * - CGI process pipe handling for async script execution
 * - Connection timeouts and cleanup
 *
 * Architecture overview:
 * ```
 *   poll() event loop
 *        │
 *   ┌────┴────┐
 *   │         │
 * Server   Client
 * sockets  sockets/CGI pipes
 *   │         │
 * accept  read/write/CGI
 * ```
 *
 * The poll() array structure:
 * - Indices [0..N-1]: Server listening sockets (one per port)
 * - Indices [N..M]:   Client sockets and CGI output pipes
 *
 * Event handling flow:
 * 1. poll() waits for events (5 second timeout for periodic checks)
 * 2. Check server sockets for new connections → accept
 * 3. Check client sockets for data → read/process/write
 * 4. Check CGI pipes for output → collect and build response
 * 5. Cleanup closed connections
 *
 * Non-blocking I/O:
 * All sockets are set to O_NONBLOCK. This means:
 * - accept() returns EAGAIN when no connections pending
 * - recv() returns EAGAIN when no data available
 * - send() returns EAGAIN when buffer full (partial write)
 *
 * @see ClientConnection for per-client state management
 * @see PollManager for poll() abstraction
 * @see ServerSocket for listening socket management
 */

// Global flag for graceful shutdown (defined in main.cpp)
extern volatile sig_atomic_t g_running;

/**
 * @brief Constructor - stores virtual host configurations
 *
 * The server can handle multiple virtual hosts (server blocks) listening
 * on different ports or the same port with different server_names.
 *
 * @param servConfigsList Vector of server configurations from config parser
 */
Server::Server(const std::vector<ServerConfig> &servConfigsList)
    : _servConfigsList(servConfigsList) {}

/**
 * @brief Destructor - cleanup all resources
 *
 * Properly releases all resources:
 * 1. Delete all ClientConnection objects (triggers their destructors)
 * 2. Delete all ServerSocket objects (closes listening sockets)
 *
 * Memory ownership:
 * - Server owns ClientConnection* in _clientsByFd (via new)
 * - Server owns ServerSocket* in _serverSockets (via new)
 * - ClientConnection owns its socket fd (closes in its destructor)
 */
Server::~Server() {
  // Close all client connections
  for (std::map<int, ClientConnection *>::iterator it = _clientsByFd.begin();
       it != _clientsByFd.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }
  _clientsByFd.clear();

  // Close all server sockets
  for (size_t i = 0; i < _serverSockets.size(); ++i) {
    delete _serverSockets[i];
  }
  _serverSockets.clear();
}

/**
 * @brief Initializes server - creates listening sockets for each port
 *
 * Groups configurations by port and creates one listening socket per port.
 * Each socket is added to the poll manager for event monitoring.
 *
 * Example with multiple virtual hosts:
 *   server { listen 8080; server_name a.com; }
 *   server { listen 8080; server_name b.com; }
 *   server { listen 9090; server_name c.com; }
 *
 *   Result: 2 sockets created (port 8080 and 9090)
 *   Port 8080 handles both a.com and b.com based on Host header
 *
 * @return true if all sockets initialized successfully, false on error
 */
bool Server::init() {
  // Step 1: Group server blocks by port (virtual hosting)
  // Multiple server_names can share the same port
  std::map<int, ConfigVector> configsByPort = groupConfigsByPort();

  // Step 2: Create one listening socket per unique port
  for (std::map<int, ConfigVector>::iterator it = configsByPort.begin();
       it != configsByPort.end(); ++it) {
    int port = it->first;

    // Create and initialize the server socket (socket + bind + listen)
    ServerSocket *serverSocket = new ServerSocket(port);

    if (!serverSocket->init()) {
      std::cerr << "❌ [Error] Failed to initialize server socket on port "
                << port << std::endl;
      delete serverSocket;
      return false;
    }

    // Step 3: Store socket and associate configs with this fd
    int fd = serverSocket->getFd();
    _serverSockets.push_back(serverSocket); // Keep track of socket object
    _configsByServerFd[fd] = it->second;    // Map fd → server configs

    // Step 4: Register socket in poll manager for POLLIN events
    // When a client connects, poll() will signal this fd
    _pollManager.addFd(fd, POLLIN);

    std::cout << "🌐 Server listening on port " << port << " (fd: " << fd << ")"
              << std::endl;
  }

  return true;
}

/**
 * @brief Groups server configurations by port number
 *
 * Multiple server blocks can listen on the same port (virtual hosting).
 * This function groups them so we create only one socket per port.
 *
 * @return Map: port number → vector of ServerConfig for that port
 */
std::map<int, std::vector<ServerConfig> > Server::groupConfigsByPort() {
  std::map<int, ConfigVector> configsByPort;
  for (size_t i = 0; i < _servConfigsList.size(); ++i) {
    configsByPort[_servConfigsList[i].getListen()].push_back(
        _servConfigsList[i]);
  }
  return configsByPort;
}

/**
 * @brief Main event loop - the heart of the server
 *
 * Uses poll() for non-blocking I/O multiplexing. The loop:
 * 1. Waits for events on any registered fd (5s timeout)
 * 2. Handles server socket events (new connections)
 * 3. Handles client socket events (read/write)
 * 4. Handles CGI pipe events (async script output)
 * 5. Cleans up closed connections
 *
 * Poll event handling order (important for correctness):
 * 1. POLLERR/POLLHUP/POLLNVAL → Mark client closed immediately
 * 2. POLLIN → Read incoming data
 * 3. POLLOUT → Write pending response data
 *
 * This order prevents attempting I/O on dead sockets.
 *
 * CGI pipe handling:
 * When a CGI script runs asynchronously, its output pipe is added to poll.
 * When POLLIN fires on the pipe, we read output until EOF, then build
 * the HTTP response and queue it for sending to the client.
 */
void Server::run() {
  std::cout << "[Info] Server running with poll()..." << std::endl;

  while (g_running) {
    // Wait for events (5s timeout allows periodic timeout checks)
    int ready = _pollManager.wait(5000);
    if (ready < 0) {
      if (errno == EINTR)
        continue; // Interrupted by signal, retry
      perror("poll");
      break;
    }

    time_t now = time(NULL);

    // ===== PHASE 1: Accept new connections on server sockets =====
    for (size_t i = 0; i < _serverSockets.size(); ++i) {
      short revents = _pollManager.getRevents(i);
      if (revents & POLLIN) {
        acceptNewClient(_pollManager.getFd(i));
      }
    }

    // ===== PHASE 2: Process client sockets and CGI pipes =====
    for (size_t i = _serverSockets.size(); i < _pollManager.getSize();) {
      int fd = _pollManager.getFd(i);

      // --- CGI Pipe Check ---
      std::map<int, ClientConnection *>::iterator cgiIt =
          _cgiPipeToClient.find(fd);
      if (cgiIt != _cgiPipeToClient.end()) {
        ClientConnection *client = cgiIt->second;
        short revents = _pollManager.getRevents(i);
        if (revents & (POLLIN | POLLHUP | POLLERR)) {
          handleCGIPipe(fd, client);
        }
        ++i;
        continue;
      }

      // --- Regular Client Socket ---
      ClientConnection *client = _clientsByFd[fd];

      if (!client) {
        _pollManager.removeFd(fd);
        continue;
      }

      // Check timeout first (more efficient)
      checkClientTimeout(client, fd, now);

      if (!client->isClosed()) {
        short revents = _pollManager.getRevents(i);

        // Handle errors first (before attempting I/O)
        if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
          client->markClosed();
          ++i;
          continue;
        }

        // Handle incoming data
        if (revents & POLLIN) {
          handleClientData(client, i);
        }

        // Handle outgoing data
        if (revents & POLLOUT) {
          handleClientWrite(client, i);
          if (client->isClosed()) {
            ++i;
            continue;
          }
        }
      }

      // Advance index if we didn't remove this fd
      if (i < _pollManager.getSize() && _pollManager.getFd(i) == fd) {
        // Pipelining note: if client has more data in buffer but request not
        // yet complete and no pending writes, the next poll() iteration will
        // continue processing. handleClientData() already loops on complete
        // requests via checkForNextRequest().
        ++i;
      }
    }

    // ===== PHASE 3: Cleanup closed connections =====
    cleanupClosedClients();
  }
}

/**
 * @brief Checks if a client has timed out due to inactivity
 *
 * Clients with no activity for CLIENT_TIMEOUT seconds are closed.
 * Exception: clients with pending write data are not timed out.
 *
 * @param client The client connection to check
 * @param fd Client's file descriptor (for logging)
 * @param now Current timestamp
 */
void Server::checkClientTimeout(ClientConnection *client, int fd, time_t now) {
  const int CLIENT_TIMEOUT = 30;

  // Don't timeout if we're still sending data
  if (client->hasPendingWrite()) {
    return;
  }

  if (client->isTimedOut(now, CLIENT_TIMEOUT)) {
    std::cout << "⚠️ [Timeout] Client fd " << fd << " inactive for "
              << CLIENT_TIMEOUT << "s, closing." << std::endl;
    client->markClosed();
  }
}

/**
 * @brief Accepts new client connections from a server socket
 *
 * Called when poll() indicates POLLIN on a server socket.
 * Uses a loop because multiple connections may be pending.
 *
 * For each new client:
 * 1. accept() creates a new socket fd for the client
 * 2. Set socket to non-blocking mode
 * 3. Create ClientConnection object with appropriate configs
 * 4. Add to poll manager for event monitoring
 *
 * @param serverFd The server socket that has pending connections
 */
void Server::acceptNewClient(int serverFd) {
  while (true) {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break; // No more pending connections
      perror("accept");
      break;
    }

    // Set non-blocking mode
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
      std::cerr << "❌ [Error] Failed to set non-blocking mode: "
                << strerror(errno) << std::endl;
      close(clientFd);
      continue;
    }

    // Create client with configs for this server socket
    ClientConnection *client = new ClientConnection(
        clientFd, clientAddr, _configsByServerFd[serverFd]);
    _clientsByFd[clientFd] = client;

    _pollManager.addFd(clientFd, POLLIN);

    std::cout << "✅ [Info] New connection (fd: " << clientFd
              << ", IP: " << client->getIp() << ")" << std::endl;
  }
}

/**
 * @brief Handles incoming data from a client socket
 *
 * Called when poll() indicates POLLIN on a client socket.
 * Implements HTTP pipelining - multiple requests can be in the buffer.
 *
 * Flow:
 * 1. Read available data into client's buffer
 * 2. While a complete request is in buffer:
 *    a. Parse and process the request
 *    b. Generate and queue response
 *    c. If CGI async, break and wait for pipe
 *    d. Check for next pipelined request
 * 3. If data pending to write, enable POLLOUT
 *
 * @param client The client with data to read
 * @param pollIndex Index in poll array (for event updates)
 */
void Server::handleClientData(ClientConnection *client, size_t pollIndex) {
  // 1. Read data from socket
  if (!client->readRequest())
    return; // Error or disconnect, cleanup will handle

  // 2. Process requests (loop for pipelining support)
  while (client->isRequestComplete()) {
    if (!client->processRequest() || !client->sendResponse())
      return; // Error, client marked closed

    // CGI async registration
    if (client->getCGIState() == CGI_RUNNING) {
      int pipeFd = client->getCGIPipeFd();
      if (pipeFd != -1 &&
          _cgiPipeToClient.find(pipeFd) == _cgiPipeToClient.end()) {
        _pollManager.addFd(pipeFd, POLLIN);
        _cgiPipeToClient[pipeFd] = client;
      }
      break; // Wait for CGI to complete before processing next request
    }

    // Check for next pipelined request
    if (!client->checkForNextRequest()) {
      break;
    }
  }

  // 3. Enable POLLOUT if we have data to send
  if (client->hasPendingWrite()) {
    _pollManager.updateEventsByIndex(pollIndex, POLLIN | POLLOUT);
  }
}

/**
 * @brief Handles outgoing data to a client socket
 *
 * Called when poll() indicates POLLOUT on a client socket.
 * Attempts to flush the write buffer to the socket.
 *
 * When buffer is fully sent, POLLOUT is disabled to avoid
 * busy-polling (sockets are almost always writable).
 *
 * @param client The client with data to write
 * @param pollIndex Index in poll array (for event updates)
 */
void Server::handleClientWrite(ClientConnection *client, size_t pollIndex) {
  client->updateActivity();

  if (!client->flushWrite())
    return; // Error, client marked closed

  // Disable POLLOUT when nothing left to send
  if (!client->hasPendingWrite()) {
    _pollManager.updateEventsByIndex(pollIndex, POLLIN);
  }
}

/**
 * @brief Removes closed client connections and frees resources
 *
 * Iterates through all clients and removes those marked as closed.
 * Also cleans up any associated CGI pipes.
 *
 * Safe iteration: uses post-increment erase pattern to avoid
 * iterator invalidation.
 */
void Server::cleanupClosedClients() {
  for (std::map<int, ClientConnection *>::iterator it = _clientsByFd.begin();
       it != _clientsByFd.end();) {
    int fd = it->first;
    ClientConnection *client = it->second;

    if (client->isClosed()) {
      std::cout << "[Info] Closing connection fd: " << fd << std::endl;

      // Cleanup associated CGI pipe if any
      int pipeFd = client->getCGIPipeFd();
      if (pipeFd != -1) {
        _pollManager.removeFd(pipeFd);
        _cgiPipeToClient.erase(pipeFd);
      }

      _pollManager.removeFd(fd);
      delete client;
      _clientsByFd.erase(it++);
    } else {
      ++it;
    }
  }
}

/**
 * @brief Handles CGI pipe data when poll() detects POLLIN
 *
 * Called when there's data to read from a running CGI process.
 * Reads available data until EOF, then builds HTTP response.
 *
 * Flow:
 * 1. Read data from pipe (non-blocking)
 * 2. When EOF reached (CGI done):
 *    a. Reap zombie process with waitpid(WNOHANG)
 *    b. Remove pipe from poll and tracking
 *    c. Parse CGI output and build HTTP response
 *    d. Queue response and enable POLLOUT
 *
 * @param pipeFd The CGI output pipe with data
 * @param client The client waiting for CGI response
 */
void Server::handleCGIPipe(int pipeFd, ClientConnection *client) {
  if (!client || client->getCGIState() != CGI_RUNNING) {
    return;
  }

  // Read available data
  bool readOk = client->readCGIOutput();

  // Check if CGI is done (EOF reached)
  if (client->getCGIState() == CGI_DONE) {
    // Reap zombie process
    pid_t pid = client->getCGIPid();
    if (pid > 0) {
      int status;
      waitpid(pid, &status, WNOHANG);
    }

    // Remove pipe from poll
    _pollManager.removeFd(pipeFd);
    _cgiPipeToClient.erase(pipeFd);

    // Build HTTP response from CGI output
    CGIHandler cgiHandler;
    HttpResponse response =
        cgiHandler.buildResponseFromCGIOutput(client->getCGIBuffer());

    // Queue response for sending
    std::string responseStr = response.buildResponse();
    client->setCGIResponse(responseStr);

    // Activate POLLOUT
    int clientFd = client->getFd();
    _pollManager.updateEvents(clientFd, POLLIN | POLLOUT);
  }
  (void)readOk;
}
