#pragma once

#include <map>
#include <string>

/**
 * @brief HTTP request parser - extracts method, path, headers, and body
 */
class HttpRequest {
public:
  HttpRequest();

  /** @brief Parse raw request data, returns true if request is complete */
  bool parse(const std::string &rawRequest);

  // Getters
  const std::string &getMethod() const;
  const std::string &getPath() const;
  const std::string &getQuery() const;
  const std::string &getVersion() const;
  const std::string &getBody() const;
  const std::map<std::string, std::string> &getHeaders() const;
  std::string getOneHeader(const std::string &key) const;
  int getParsedBytes() const;
  const std::map<std::string, std::string> &getCookies() const;

  bool headersComplete() const;
  bool isChunked() const;
  bool isKeepAlive() const;
  bool isMalformed() const;
  int getContentLength() const;

  /** @brief Reset state for reuse (keep-alive pipelining) */
  void reset();

private:
  bool _headersComplete;
  bool _isChunked;
  bool _keepAlive;
  bool _isMalformed;
  int _parsedBytes;

  std::string _method;
  std::string _path;
  std::string _query;
  std::string _version;
  std::map<std::string, std::string> _headers;
  std::map<std::string, std::string> _cookies;
  std::string _body;
  int _contentLength;

  bool parseHeaders(const std::string &rawRequest);
  bool parseBody(const std::string &rawRequest);
  bool parseChunkedBody(const std::string &chunkedData);
  void _parseCookies();
  std::string _urlDecode(const std::string &encoded, bool plusAsSpace) const;
};