#include "../../includes/config/LocationConfig.hpp"

LocationConfig::LocationConfig() : _returnCode(0), _bodySize(1048576), _autoindex(false)
{
}

LocationConfig::LocationConfig(const LocationConfig &other)
    : _root(other._root), _index(other._index), _methods(other._methods),
      _cgiPaths(other._cgiPaths), _cgiExts(other._cgiExts), _errorPages(other._errorPages),
      _returnCode(other._returnCode), _returnUrl(other._returnUrl),
      _bodySize(other._bodySize), _pattern(other._pattern), _uploadPath(other._uploadPath),
      _autoindex(other._autoindex)
{
}

LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
    if (this != &other)
    {
        _root = other._root;
        _index = other._index;
        _methods = other._methods;
        _cgiPaths = other._cgiPaths;
        _cgiExts = other._cgiExts;
        _errorPages = other._errorPages;
        _returnCode = other._returnCode;
        _returnUrl = other._returnUrl;
        _bodySize = other._bodySize;
        _pattern = other._pattern;
        _uploadPath = other._uploadPath;
        _autoindex = other._autoindex;
    }
    return *this;
}

LocationConfig::~LocationConfig()
{
}

const std::string &LocationConfig::getRoot() const
{
    return _root;
}

const std::vector<std::string> &LocationConfig::getIndex() const
{
    return _index;
}

const std::vector<std::string> &LocationConfig::getMethods() const
{
    return _methods;
}

const std::vector<std::string> &LocationConfig::getCgiPaths() const
{
    return _cgiPaths;
}

const std::vector<std::string> &LocationConfig::getCgiExts() const
{
    return _cgiExts;
}

const std::map<int, std::string> &LocationConfig::getErrorPages() const
{
    return _errorPages;
}

int LocationConfig::getReturnCode() const
{
    return _returnCode;
}

const std::string &LocationConfig::getReturnUrl() const
{
    return _returnUrl;
}

size_t LocationConfig::getBodySize() const
{
    return _bodySize;
}

const std::string &LocationConfig::getPattern() const
{
    return _pattern;
}

const std::string &LocationConfig::getUploadPath() const
{
    return _uploadPath;
}

bool LocationConfig::getAutoindex() const
{
    return _autoindex;
}

void LocationConfig::setRoot(const std::string &root)
{
    _root = root;
}

void LocationConfig::setIndex(const std::vector<std::string> &index)
{
    _index = index;
}

void LocationConfig::setMethods(const std::vector<std::string> &methods)
{
    _methods = methods;
}

void LocationConfig::setCgiPaths(const std::vector<std::string> &cgiPaths)
{
    _cgiPaths = cgiPaths;
}

void LocationConfig::setCgiExts(const std::vector<std::string> &cgiExts)
{
    _cgiExts = cgiExts;
}

void LocationConfig::setErrorPages(const std::map<int, std::string> &errorPages)
{
    _errorPages = errorPages;
}

void LocationConfig::setReturnCode(int returnCode)
{
    _returnCode = returnCode;
}

void LocationConfig::setReturnUrl(const std::string &returnUrl)
{
    _returnUrl = returnUrl;
}

void LocationConfig::setBodySize(size_t bodySize)
{
    _bodySize = bodySize;
}

void LocationConfig::setPattern(const std::string &pattern)
{
    _pattern = pattern;
}

void LocationConfig::setUploadPath(const std::string &uploadPath)
{
    _uploadPath = uploadPath;
}

void LocationConfig::setAutoindex(bool autoindex)
{
    _autoindex = autoindex;
}
