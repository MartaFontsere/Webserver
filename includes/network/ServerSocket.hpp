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
 * @brief Encapsula un socket de escucha del servidor.
 *
 * Se encarga de la creación, configuración (setsockopt, non-blocking),
 * bind y listen de un socket asociado a un puerto.
 */
class ServerSocket {
private:
  int _fd;
  int _port;

  int setNonBlocking(int fd);

public:
  ServerSocket(int port);
  ~ServerSocket();

  bool init();
  int getFd() const;
  int getPort() const;
  void closeSocket();
};
