#include "../../includes/config/LocationConfig.hpp"

LocationConfig::LocationConfig() : _returnCode(0), _bodySize(1048576), _autoindex(false)
{
}

LocationConfig::LocationConfig(const LocationConfig &other)
    : _root(other._root), _index(other._index), _methods(other._methods),
      _cgiPaths(other._cgiPaths), _cgiExts(other._cgiExts), _errorPage(other._errorPage),
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
        _errorPage = other._errorPage;
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