#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>

class LocationConfig
{
private:
    std::string _root;
    std::vector<std::string> _index;
    std::vector<std::string> _methods;
    std::vector<std::string> _cgiPaths;
    std::vector<std::string> _cgiExts;
    std::map<int, std::string> _errorPage;
    int _returnCode;
    std::string _returnUrl;
    size_t _bodySize;
    std::string _pattern;
    std::string _uploadPath;
    bool _autoindex;

public:
    LocationConfig();
    LocationConfig(const LocationConfig &other);
    ~LocationConfig();

    LocationConfig &operator=(const LocationConfig &other);
};

#endif