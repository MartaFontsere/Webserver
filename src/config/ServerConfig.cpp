#include "../../includes/config/ServerConfig.hpp"

ServerConfig::ServerConfig() : _listen(0), _clientMaxBodySize(1048576)
{
}

ServerConfig::ServerConfig(const ServerConfig &other)
    : _listen(other._listen), _host(other._host), _serverNames(other._serverNames),
      _root(other._root), _index(other._index), _errorPages(other._errorPages),
      _clientMaxBodySize(other._clientMaxBodySize), _locations(other._locations)
{
}

ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        _listen = other._listen;
        _host = other._host;
        _serverNames = other._serverNames;
        _root = other._root;
        _index = other._index;
        _errorPages = other._errorPages;
        _clientMaxBodySize = other._clientMaxBodySize;
        _locations = other._locations;
    }
    return *this;
}

ServerConfig::~ServerConfig()
{
}

int ServerConfig::getListen() const
{
    return _listen;
}

const std::string &ServerConfig::getHost() const
{
    return _host;
}
const std::vector<std::string> &ServerConfig::getServerNames() const
{
    return _serverNames;
}

const std::string &ServerConfig::getRoot() const
{
    return _root;
}

const std::vector<std::string> &ServerConfig::getIndex() const
{
    return _index;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const
{
    return _errorPages;
}

size_t ServerConfig::getClientMaxBodySize() const
{
    return _clientMaxBodySize;
}

const std::vector<LocationConfig> &ServerConfig::getLocations() const
{
    return _locations;
}

void ServerConfig::setListen(int listen)
{
    _listen = listen;
}

void ServerConfig::setHost(const std::string &host)
{
    _host = host;
}

void ServerConfig::setServerNames(const std::vector<std::string> &serverNames)
{
    _serverNames = serverNames;
}

void ServerConfig::setRoot(const std::string &root)
{
    _root = root;
}

void ServerConfig::setIndex(const std::vector<std::string> &index)
{
    _index = index;
}

void ServerConfig::setErrorPages(const std::map<int, std::string> &errorPages)
{
    _errorPages = errorPages;
}

void ServerConfig::setClientMaxBodySize(size_t clientMaxBodySize)
{
    _clientMaxBodySize = clientMaxBodySize;
}

void ServerConfig::setLocations(const std::vector<LocationConfig> &locations)
{
    _locations = locations;
}