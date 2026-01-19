#include "network/ServerSocket.hpp"

/**
 * @file ServerSocket.cpp
 * @brief Server listening socket creation and configuration
 *
 * This module encapsulates the creation and configuration of a TCP server
 * socket, handling all POSIX socket setup steps:
 * 1. Socket creation (socket())
 * 2. Address reuse configuration (setsockopt SO_REUSEADDR)
 * 3. Non-blocking mode (fcntl O_NONBLOCK)
 * 4. Binding to address/port (bind())
 * 5. Listening for connections (listen())
 *
 * @note Uses IPv4 (AF_INET) with TCP (SOCK_STREAM)
 * @see socket(2), bind(2), listen(2) man pages
 */

/**
 * @brief Constructor with port specification
 *
 * Initializes a ServerSocket object with the specified port.
 * The actual socket is not created until init() is called.
 *
 * @param port Port number to listen on
 *
 * @note _fd is set to -1 (invalid) until init() succeeds
 */
ServerSocket::ServerSocket(int port) : _fd(-1), _port(port) {}

/**
 * @brief Destructor
 *
 * Ensures the socket is properly closed when the object is destroyed.
 * Calls closeSocket() to release the file descriptor.
 */
ServerSocket::~ServerSocket() { closeSocket(); }

/**
 * @brief Initializes the server socket
 *
 * Performs all necessary steps to create a listening TCP socket:
 *
 * Step 1: Create socket
 *   - AF_INET: IPv4 address family
 *   - SOCK_STREAM: TCP (connection-oriented)
 *   - Protocol 0: Default for TCP
 *
 * Step 2: Configure SO_REUSEADDR
 *   - Allows restarting server without waiting for TIME_WAIT to expire
 *   - Prevents "Address already in use" errors on quick restart
 *
 * Step 3: Set non-blocking mode
 *   - Required for poll()-based I/O multiplexing
 *   - Prevents accept() from blocking when no connections pending
 *
 * Step 4: Bind to address
 *   - INADDR_ANY: Accept connections on any network interface
 *   - sin_port: Network byte order via htons()
 *
 * Step 5: Start listening
 *   - SOMAXCONN: Maximum pending connection queue size
 *
 * @return true if socket initialized successfully, false on error
 *
 * @note Errors are logged to stderr with port number
 * @note On failure, socket is closed to prevent resource leaks
 */
bool ServerSocket::init() {
  // Step 1: Create socket (IPv4, TCP)
  _fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_fd < 0) {
    std::cerr << "❌ Error creating socket on port " << _port << ": "
              << strerror(errno) << std::endl;
    return false;
  }

  // Step 2: Configure SO_REUSEADDR to allow quick server restart
  int opt = 1;
  if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "❌ Error configuring SO_REUSEADDR on port " << _port << ": "
              << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }

  // Step 3: Set non-blocking mode for poll() compatibility
  if (setNonBlocking(_fd) < 0) {
    std::cerr << "❌ Error setting non-blocking mode on port " << _port << ": "
              << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }

  // Step 4: Configure address structure
  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(_port);

  // Step 5: Bind socket to address
  if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    std::cerr << "❌ Error binding to port " << _port << ": " << strerror(errno)
              << std::endl;
    closeSocket();
    return false;
  }

  // Step 6: Start listening for connections
  if (listen(_fd, SOMAXCONN) < 0) {
    std::cerr << "❌ Error listening on port " << _port << ": "
              << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }

  return true;
}

/**
 * @brief Sets a file descriptor to non-blocking mode
 *
 * Uses fcntl() to add O_NONBLOCK flag to the file descriptor flags.
 * Required for poll()-based I/O multiplexing.
 *
 * @param fd File descriptor to modify
 * @return 0 on success, -1 on error
 *
 * @note Uses F_GETFL/F_SETFL to preserve existing flags
 */
int ServerSocket::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Returns the socket file descriptor
 *
 * @return Socket fd, or -1 if not initialized or closed
 */
int ServerSocket::getFd() const { return _fd; }

/**
 * @brief Returns the port number
 *
 * @return Port this socket is configured for
 */
int ServerSocket::getPort() const { return _port; }

/**
 * @brief Closes the socket
 *
 * Releases the file descriptor if open. Safe to call multiple times.
 * Sets _fd to -1 after closing.
 */
void ServerSocket::closeSocket() {
  if (_fd != -1) {
    close(_fd);
    _fd = -1;
  }
}
