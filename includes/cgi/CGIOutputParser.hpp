#ifndef CGIOUTPUTPARSER_HPP
#define CGIOUTPUTPARSER_HPP

#include <map>
#include <sstream>
#include <string>
#include <vector>

class CGIOutputParser {
private:
  std::map<std::string, std::string> _headers;
  std::vector<std::string> _setCookies;
  std::string _body;
  int _statusCode;

public:
  CGIOutputParser();
  ~CGIOutputParser();

  void parse(const std::string &rawOutput);

  std::map<std::string, std::string> getHeaders() const;
  std::vector<std::string> getSetCookies() const;
  std::string getBody() const;
  int getStatusCode() const;
};

#endif