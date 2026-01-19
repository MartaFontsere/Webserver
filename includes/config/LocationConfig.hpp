#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <map>
#include <string>
#include <vector>

class LocationConfig {
private:
  std::string _root;
  std::vector<std::string> _index;
  std::vector<std::string> _methods;
  std::vector<std::string> _cgiPaths;
  std::vector<std::string> _cgiExts;
  std::map<int, std::string> _errorPages;
  int _returnCode;
  std::string _returnUrl;
  size_t _maxBodySize;
  std::string _pattern;
  std::string _uploadPath;
  std::string _alias;
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
  const std::map<int, std::string> &getErrorPages() const;
  int getReturnCode() const;
  const std::string &getReturnUrl() const;
  size_t getMaxBodySize() const;
  const std::string &getPattern() const;
  bool isMethodAllowed(const std::string &method) const;
  bool isUploadEnabled() const;
  const std::string &getUploadPath() const;
  bool hasAlias() const;
  const std::string &getAlias() const;
  bool getAutoindex() const;

  void setRoot(const std::string &root);
  void setIndex(const std::vector<std::string> &index);
  void setMethods(const std::vector<std::string> &methods);
  void setCgiPaths(const std::vector<std::string> &cgiPaths);
  void setCgiExts(const std::vector<std::string> &cgiExts);
  void setErrorPages(const std::map<int, std::string> &errorPage);
  void setReturnCode(int returnCode);
  void setReturnUrl(const std::string &returnUrl);
  void setMaxBodySize(size_t maxBodySize);
  void setPattern(const std::string &pattern);
  void setUploadPath(const std::string &uploadPath);
  void setAlias(const std::string &alias);
  void setAutoindex(bool autoindex);
};

#endif