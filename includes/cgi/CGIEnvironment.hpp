#ifndef CGIENVIRONMENT_HPP
#define CGIENVIRONMENT_HPP

#include "http/HttpRequest.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class CGIEnvironment {
private:
  std::map<std::string, std::string> _envVars;

public:
  CGIEnvironment();
  ~CGIEnvironment();

  void prepare(const HttpRequest &req, const std::string &scriptPath,
               const std::string &scriptName, const std::string &serverName,
               int serverPort);

  char **toEnvArray() const;
  void freeEnvArray(char **env) const;

  std::string getVar(const std::string &key) const;
  void printAll() const;
};

#endif