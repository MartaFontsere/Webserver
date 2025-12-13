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

    const std::string &getRoot() const;
    const std::vector<std::string> &getIndex() const;
    const std::vector<std::string> &getMethods() const;
    const std::vector<std::string> &getCgiPaths() const;
    const std::vector<std::string> &getCgiExts() const;
    const std::map<int, std::string> &getErrorPage() const;
    int getReturnCode() const;
    const std::string &getReturnUrl() const;
    size_t getBodySize() const;
    const std::string &getPattern() const;
    const std::string &getUploadPath() const;
    bool getAutoindex() const;

    void setRoot(const std::string &root);
    void setIndex(const std::vector<std::string> &index);
    void setMethods(const std::vector<std::string> &methods);
    void setCgiPaths(const std::vector<std::string> &cgiPaths);
    void setCgiExts(const std::vector<std::string> &cgiExts);
    void setErrorPage(const std::map<int, std::string> &errorPage);
    void setReturnCode(int returnCode);
    void setReturnUrl(const std::string &returnUrl);
    void setBodySize(size_t bodySize);
    void setPattern(const std::string &pattern);
    void setUploadPath(const std::string &uploadPath);
    void setAutoindex(bool autoindex);
};

#endif