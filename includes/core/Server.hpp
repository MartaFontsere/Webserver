#pragma once

#include "config/ServerConfig.hpp"
#include "network/ClientConnection.hpp"
#include "network/PollManager.hpp"
#include "network/ServerSocket.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Main server class - event loop and connection management
 */
class Server {
private:
  std::vector<ServerConfig> _servConfigsList;
  std::vector<ServerSocket *> _serverSockets;
  PollManager _pollManager;

  typedef std::vector<ServerConfig> ConfigVector;
  std::map<int, ConfigVector> _configsByServerFd;
  std::map<int, ClientConnection *> _clientsByFd;
  std::map<int, ClientConnection *> _cgiPipeToClient;

  void acceptNewClient(int serverFd);
  void handleClientData(ClientConnection *client, size_t pollIndex);
  void handleClientWrite(ClientConnection *client, size_t pollIndex);
  void handleCGIPipe(int pipeFd, ClientConnection *client);
  void checkClientTimeout(ClientConnection *client, int fd, time_t now);
  void cleanupClosedClients();
  std::map<int, ConfigVector> groupConfigsByPort();

public:
  Server(const std::vector<ServerConfig> &configs);
  ~Server();

  /** @brief Initialize listening sockets for all configured ports */
  bool init();

  /** @brief Run main poll() event loop until shutdown */
  void run();
};
