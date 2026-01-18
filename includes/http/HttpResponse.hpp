#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * @brief HTTP response builder - assembles status line, headers, and body
 */
class HttpResponse {
private:
  int _statusCode;
  std::string _statusMessage;
  std::string _httpVersion;
  std::map<std::string, std::string> _headers;
  std::vector<std::string> _setCookies;
  std::string _body;
  bool _cgiPending;

public:
  HttpResponse();
  ~HttpResponse();

  void setStatus(int code, const std::string &message);
  void setHeader(const std::string &key, const std::string &value);
  void setCookie(const std::string &cookie);
  void setBody(const std::string &body);
  int getStatusCode() const;

  void setCGIPending(bool pending);
  bool isCGIPending() const;

  /** @brief Build final HTTP response string with headers and body */
  std::string buildResponse() const;

  /** @brief Set error response with default error page */
  void setErrorResponse(int code);

  static std::string getHttpStatusMessage(int code);
};