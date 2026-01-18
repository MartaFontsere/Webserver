#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Server block configuration - virtual host settings
 */
class ServerConfig {
private:
  int _listen;
  std::string _host;
  std::vector<std::string> _serverNames;
  std::string _root;
  std::vector<std::string> _index;
  std::map<int, std::string> _errorPages;
  size_t _clientMaxBodySize;
  std::vector<LocationConfig> _locations;

public:
  ServerConfig();
  ServerConfig(const ServerConfig &other);
  ~ServerConfig();

  ServerConfig &operator=(const ServerConfig &other);

  int getListen() const;
  const std::string &getHost() const;
  const std::vector<std::string> &getServerNames() const;
  const std::string &getRoot() const;
  const std::vector<std::string> &getIndex() const;
  const std::map<int, std::string> &getErrorPages() const;
  size_t getClientMaxBodySize() const;
  const std::vector<LocationConfig> &getLocations() const;

  void setListen(int listen);
  void setHost(const std::string &host);
  void setServerNames(const std::vector<std::string> &serverNames);
  void setRoot(const std::string &root);
  void setIndex(const std::vector<std::string> &index);
  void setErrorPages(const std::map<int, std::string> &errorPages);
  void setClientMaxBodySize(size_t clientMaxBodySize);
  void setLocations(const std::vector<LocationConfig> &locations);
};

#endif