#pragma once

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

/**
 * @brief Listening socket wrapper - handles bind, listen, and non-blocking
 * setup
 */
class ServerSocket {
private:
  int _fd;
  int _port;

  int setNonBlocking(int fd);

public:
  ServerSocket(int port);
  ~ServerSocket();

  /** @brief Create socket, bind to port, and start listening */
  bool init();
  int getFd() const;
  int getPort() const;
  void closeSocket();
};
