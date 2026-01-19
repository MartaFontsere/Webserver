#include "network/ClientConnection.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @file ClientConnection.cpp
 * @brief Client connection state management and I/O handling
 *
 * This module manages individual client connections to the server. Each
 * ClientConnection object encapsulates:
 * - Socket file descriptor and client address
 * - Raw request data buffer and parsed HttpRequest
 * - Response data buffer and write progress
 * - CGI execution state (for async CGI handling)
 * - Keep-alive and pipelining support
 *
 * The connection follows this lifecycle:
 * 1. Created by Server when accept() returns new client fd
 * 2. readRequest() accumulates data until HTTP request is complete
 * 3. processRequest() generates HttpResponse via RequestHandler
 * 4. sendResponse()/flushWrite() sends response data
 * 5. If keep-alive: resetForNextRequest() and repeat from step 2
 * 6. Destructor cleans up socket and any running CGI process
 *
 * @note Does not check errno after recv/send per subject requirements
 * @see HttpRequest for request parsing
 * @see RequestHandler for request processing
 */

/**
 * @brief Constructor with socket, address, and server configurations
 *
 * Creates a new client connection with the accepted socket and potential
 * server configurations for this port.
 *
 * @param fd Client socket file descriptor from accept()
 * @param addr Client address structure from accept()
 * @param servCandidateConfigs Server configs matching the listening port
 *
 * @note The final ServerConfig is selected later based on Host header
 */
ClientConnection::ClientConnection(
    int fd, const sockaddr_in &addr,
    const std::vector<ServerConfig> &servCandidateConfigs)
    : _clientFd(fd), _addr(addr), _closed(false), _rawRequest(""),
      _writeBuffer(""), _writeOffset(0), _lastActivity(time(NULL)),
      _requestComplete(false), _servCandidateConfigs(servCandidateConfigs),
      _cgiState(CGI_NONE), _cgiPipeFd(-1), _cgiPid(0) {}

/**
 * @brief Destructor - cleans up connection resources
 *
 * Performs cleanup in this order:
 * 1. Kill any running CGI process (SIGKILL + waitpid)
 * 2. Close CGI pipe if open
 * 3. Close client socket
 */
ClientConnection::~ClientConnection() {
  // Cleanup CGI process if running
  if (_cgiPid > 0) {
    std::cout << "[Info] Killing CGI process " << _cgiPid << " for fd "
              << _clientFd << std::endl;
    kill(_cgiPid, SIGKILL);
    int status;
    waitpid(_cgiPid, &status, 0);
  }

  // Close CGI pipe if open
  if (_cgiPipeFd != -1) {
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
  }

  // Close client socket
  if (_clientFd != -1) {
    std::cout << "[Info] Closing connection with " << getIp()
              << " (fd: " << _clientFd << ")" << std::endl;
    close(_clientFd);
    _clientFd = -1;
  }
  _closed = true;
}

/**
 * @brief Returns the client socket file descriptor
 *
 * @return Client fd, or -1 if closed
 */
int ClientConnection::getFd() const { return _clientFd; }

/**
 * @brief Returns the client IP address as a string
 *
 * Converts the sockaddr_in to a human-readable IP string.
 *
 * @return IP address string (e.g., "192.168.1.100"), or "Unknown IP" on error
 */
std::string ClientConnection::getIp() const {
  char ipStr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &_addr.sin_addr, ipStr, sizeof(ipStr)) != NULL)
    return std::string(ipStr);
  return "Unknown IP";
}

/**
 * @brief Reads data from the client socket
 *
 * Reads available data from the socket into the request buffer.
 * Attempts to parse the request after each read.
 *
 * Error handling (per subject requirement - no errno checking):
 * - bytesRead < 0: Treat as error, mark connection closed
 * - bytesRead == 0: Client closed connection gracefully
 * - bytesRead > 0: Append to buffer and try parsing
 *
 * @return true if read successful or request incomplete, false on error/close
 *
 * @note Should only be called when poll() indicates POLLIN
 * @note Supports HTTP pipelining by preserving unparsed data
 */
bool ClientConnection::readRequest() {
  char buffer[4096];
  int bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);

  if (bytesRead < 0) {
    // poll() indicated POLLIN but recv() failed - real error
    // (errno not checked per subject requirement)
    std::cerr << "❌ [Error] recv() failed for client fd " << _clientFd << "\n";
    _closed = true;
    return false;
  } else if (bytesRead == 0) {
    // Client closed connection gracefully
    std::cout << "[Info] Client closed connection (fd: " << _clientFd << ")\n";
    _closed = true;
    return false;
  }

  // bytesRead > 0: Append data to request buffer
  std::cout << "\n[Info] Reading request (fd: " << _clientFd << ")\n";
  _rawRequest.append(buffer, bytesRead);

  std::cout << _rawRequest;

  _lastActivity = time(NULL);

  // Try to parse the accumulated request data
  std::cout << "[Debug] Parsing request from client fd " << _clientFd
            << std::endl;
  if (_httpRequest.parse(_rawRequest)) {
    std::cout << "✅ [Info] Request complete (fd: " << _clientFd << ")\n";
    _requestComplete = true;

    // Pipelining support: remove only parsed bytes, keep remainder
    int parsedBytes = _httpRequest.getParsedBytes();
    if (parsedBytes > 0 && (size_t)parsedBytes <= _rawRequest.size()) {
      _rawRequest.erase(0, parsedBytes);
      std::cout << "[Debug] Pipelining: erased " << parsedBytes
                << " bytes. Remaining in buffer: " << _rawRequest.size()
                << std::endl;
    } else {
      _rawRequest.clear();
    }
  } else if (_httpRequest.headersComplete()) {
    // Early body size check using Content-Length header
    if (_httpRequest.getContentLength() > 0) {
      size_t maxBody = 1024 * 1024; // 1MB default
      if (!_servCandidateConfigs.empty()) {
        maxBody = _servCandidateConfigs[0].getClientMaxBodySize();
      }

      if ((size_t)_httpRequest.getContentLength() > maxBody) {
        std::cout << "⚠️ [Warning] Body too large ("
                  << _httpRequest.getContentLength() << " > " << maxBody
                  << "). Stopping read.\n";
        _requestComplete = true; // Force completion for 413 response
      }
    }
  }
  return true;
}

/**
 * @brief Processes a complete HTTP request
 *
 * Delegates request handling to RequestHandler and prepares the response
 * for sending. For CGI requests, may return early with pending state.
 *
 * @return true on success (includes CGI pending state)
 *
 * @note Only processes if _requestComplete is true
 * @note For CGI requests, response is set later via setCGIResponse()
 */
bool ClientConnection::processRequest() {
  if (!_requestComplete)
    return true;

  // Guard: Don't reprocess if CGI is already running
  if (_cgiState != CGI_NONE)
    return true;

  // Process request through handler
  _httpResponse =
      _requestHandler.handleRequest(_httpRequest, _servCandidateConfigs, this);

  // If CGI is pending, wait for async completion
  if (_httpResponse.isCGIPending()) {
    std::cout << "[CGI] Pending for fd: " << _clientFd << std::endl;
    return true;
  }

  // Build response for non-CGI or sync CGI requests
  _writeBuffer = _httpResponse.buildResponse();
  _writeOffset = 0;

  return true;
}

/**
 * @brief Initiates response sending
 *
 * Wrapper that calls flushWrite() to send pending response data.
 *
 * @return Result of flushWrite()
 */
bool ClientConnection::sendResponse() { return flushWrite(); }

/**
 * @brief Sends pending response data to the client
 *
 * Attempts to send all remaining data in _writeBuffer. Handles partial
 * sends by updating _writeOffset.
 *
 * Error handling (per subject requirement - no errno checking):
 * - s > 0: Data sent successfully
 * - s == -1: Treat as error, mark connection closed
 * - s == 0: Peer closed connection
 *
 * After complete send:
 * - If !keep-alive: Mark connection closed
 * - If keep-alive: Reset for next request
 *
 * @return true if send successful or buffer empty, false on error
 *
 * @note Should only be called when poll() indicates POLLOUT
 */
bool ClientConnection::flushWrite() {
  if (_writeBuffer.empty())
    return true;

  // Safety check: all data already sent
  if (_writeOffset >= _writeBuffer.size()) {
    _writeBuffer.clear();
    _writeOffset = 0;
    return true;
  }

  const char *buf = _writeBuffer.data() + _writeOffset;
  size_t remaining = _writeBuffer.size() - _writeOffset;

  ssize_t s = send(_clientFd, buf, remaining, 0);

  if (s > 0) {
    _writeOffset += static_cast<size_t>(s);
    _lastActivity = time(NULL);

    std::cout << "[Info] Sending response (fd: " << _clientFd
              << "): " << _writeOffset << "/" << _writeBuffer.size()
              << " bytes\n";

    // Check if all data sent
    if (_writeOffset >= _writeBuffer.size()) {
      _writeBuffer.clear();
      _writeOffset = 0;

      // Handle keep-alive vs close
      if (!_httpRequest.isKeepAlive()) {
        _closed = true;
        std::cout << "✅ [Info] Response sent (fd: " << _clientFd
                  << ") → Connection: close" << std::endl;
      } else {
        resetForNextRequest();
        std::cout << "✅ [Info] Response sent (fd: " << _clientFd
                  << ") → Connection: keep-alive\n    Waiting for new request"
                  << std::endl;
      }
    }
    return true;
  } else if (s == -1) {
    // poll() indicated POLLOUT but send() failed - real error
    std::cerr << "❌ [Error] send() failed for fd " << _clientFd << "\n";
    _closed = true;
    return false;
  } else { // s == 0
    // Peer closed connection during send
    std::cout << "[Info] Client closed during send (fd: " << _clientFd << ")\n";
    _closed = true;
    return false;
  }
}

/**
 * @brief Checks if there is pending data to send
 *
 * @return true if _writeBuffer has unsent data
 */
bool ClientConnection::hasPendingWrite() const { return !_writeBuffer.empty(); }

/**
 * @brief Marks the connection as closed
 */
void ClientConnection::markClosed() { _closed = true; }

/**
 * @brief Checks if the connection is closed
 *
 * @return true if connection is marked closed
 */
bool ClientConnection::isClosed() const { return _closed; }

/**
 * @brief Checks if the HTTP request is complete
 *
 * @return true if headers and body (if any) are fully received
 */
bool ClientConnection::isRequestComplete() const { return _requestComplete; }

/**
 * @brief Returns the parsed HTTP request
 *
 * @return Const reference to the HttpRequest object
 */
const HttpRequest &ClientConnection::getHttpRequest() const {
  return _httpRequest;
}

/**
 * @brief Returns the timestamp of last activity
 *
 * @return time_t of last read or write operation
 */
time_t ClientConnection::getLastActivity() const { return _lastActivity; }

/**
 * @brief Updates the last activity timestamp to now
 */
void ClientConnection::updateActivity() { _lastActivity = time(NULL); }

/**
 * @brief Checks if the connection has timed out
 *
 * @param now Current time
 * @param timeoutSec Timeout threshold in seconds
 * @return true if inactive longer than timeoutSec
 */
bool ClientConnection::isTimedOut(time_t now, int timeoutSec) const {
  return (now - _lastActivity) > timeoutSec;
}

/**
 * @brief Resets state for next request (keep-alive support)
 *
 * Clears request state while preserving connection and any pipelined
 * data in the raw request buffer.
 */
void ClientConnection::resetForNextRequest() {
  _httpRequest.reset();
  _requestComplete = false;
  // Note: _rawRequest not cleared to support pipelining
  std::cout << "[Debug] resetForNextRequest: rawRequest size remaining: "
            << _rawRequest.size() << std::endl;
  _writeBuffer.clear();
  _writeOffset = 0;

  // Reset CGI state
  _cgiState = CGI_NONE;
  if (_cgiPipeFd != -1) {
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
  }
  _cgiPid = 0;
  _cgiBuffer.clear();
}

/**
 * @brief Checks for and parses next pipelined request
 *
 * If there is remaining data in the raw request buffer from pipelining,
 * attempts to parse it as the next HTTP request.
 *
 * @return true if a complete request was found in the buffer
 */
bool ClientConnection::checkForNextRequest() {
  if (_rawRequest.empty())
    return false;

  std::cout << "[Debug] Checking for next request in buffer (size: "
            << _rawRequest.size() << ") for fd " << _clientFd << std::endl;

  _httpRequest.reset();

  if (_httpRequest.parse(_rawRequest)) {
    std::cout << "✅ [Info] Pipelined request complete (fd: " << _clientFd
              << ")\n";
    _requestComplete = true;

    // Remove parsed bytes, keep remainder for further pipelining
    int parsedBytes = _httpRequest.getParsedBytes();
    if (parsedBytes > 0 && (size_t)parsedBytes <= _rawRequest.size()) {
      _rawRequest.erase(0, parsedBytes);
      std::cout << "[Debug] Pipelining (buffer): erased " << parsedBytes
                << " bytes. Remaining: " << _rawRequest.size() << std::endl;
    } else {
      _rawRequest.clear();
    }
    return true;
  }
  return false;
}

// ==================== CGI Non-blocking Methods ====================

/**
 * @brief Returns the current CGI execution state
 *
 * @return CGI_NONE, CGI_RUNNING, or CGI_DONE
 */
CGIState ClientConnection::getCGIState() const { return _cgiState; }

/**
 * @brief Returns the CGI output pipe file descriptor
 *
 * @return Pipe fd, or -1 if no CGI running
 */
int ClientConnection::getCGIPipeFd() const { return _cgiPipeFd; }

/**
 * @brief Returns the CGI child process ID
 *
 * @return Child PID, or 0 if no CGI running
 */
pid_t ClientConnection::getCGIPid() const { return _cgiPid; }

/**
 * @brief Returns the accumulated CGI output buffer
 *
 * @return Const reference to CGI output data
 */
const std::string &ClientConnection::getCGIBuffer() const { return _cgiBuffer; }

/**
 * @brief Initiates CGI execution tracking
 *
 * Called after fork() to set up async CGI monitoring.
 *
 * @param pipeFd Read end of CGI output pipe
 * @param pid Child process ID
 */
void ClientConnection::startCGI(int pipeFd, pid_t pid) {
  _cgiState = CGI_RUNNING;
  _cgiPipeFd = pipeFd;
  _cgiPid = pid;
  _cgiBuffer.clear();
  std::cout << "[CGI] Started async CGI (pid: " << pid << ", pipe: " << pipeFd
            << ")\n";
}

/**
 * @brief Reads available CGI output from the pipe
 *
 * Called when poll() indicates POLLIN on the CGI pipe.
 *
 * @return true if read successful or EOF reached, false on error
 *
 * @note Sets _cgiState to CGI_DONE on EOF or error
 */
bool ClientConnection::readCGIOutput() {
  if (_cgiState != CGI_RUNNING || _cgiPipeFd == -1) {
    return false;
  }

  char buffer[4096];
  ssize_t bytesRead = read(_cgiPipeFd, buffer, sizeof(buffer));

  if (bytesRead > 0) {
    _cgiBuffer.append(buffer, bytesRead);
    _lastActivity = time(NULL);
    return true;
  } else if (bytesRead == 0) {
    // EOF - CGI process closed stdout
    std::cout << "[CGI] EOF reached, output size: " << _cgiBuffer.size()
              << " bytes\n";
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
    _cgiState = CGI_DONE;
    return true;
  } else {
    // bytesRead < 0: Error (errno not checked per subject requirement)
    std::cerr << "❌ [CGI] Read error on pipe\n";
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
    _cgiState = CGI_DONE;
    return false;
  }
}

/**
 * @brief Marks CGI execution as finished
 *
 * @param exitStatus Child process exit status (for logging)
 */
void ClientConnection::finishCGI(int exitStatus) {
  (void)exitStatus; // May be used for logging
  _cgiState = CGI_DONE;
  if (_cgiPipeFd != -1) {
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
  }
}

/**
 * @brief Sets the CGI response for sending
 *
 * Called by Server after CGI completes to set the response data
 * for normal POLLOUT sending flow.
 *
 * @param responseStr Complete HTTP response string
 */
void ClientConnection::setCGIResponse(const std::string &responseStr) {
  _writeBuffer = responseStr;
  _writeOffset = 0;
}
