#include "http/HttpRequest.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <strings.h>

/**
 * @file HttpRequest.cpp
 * @brief HTTP request parsing and data extraction
 *
 * This module handles progressive parsing of HTTP requests. Features:
 * - Progressive parsing (can be called multiple times as data arrives)
 * - Request line parsing (method, path, version)
 * - Header extraction with case-insensitive keys
 * - Query string separation from path
 * - URL decoding (%XX sequences and + for spaces in queries)
 * - Cookie parsing
 * - Body parsing (Content-Length and chunked transfer encoding)
 * - HTTP/1.1 pipelining support via parsed bytes tracking
 *
 * @note Body size limits are enforced in RequestHandler, not here
 * @see ClientConnection for request accumulation
 * @see RequestHandler for request processing
 */

/**
 * @brief Default constructor
 *
 * Initializes all flags and counters to their default state.
 */
HttpRequest::HttpRequest()
    : _headersComplete(false), _isChunked(false), _keepAlive(false),
      _isMalformed(false), _parsedBytes(0), _contentLength(-1) {}

/**
 * @brief Main progressive parsing function
 *
 * Called each time new data arrives. Attempts to parse headers first,
 * then body if headers are complete.
 *
 * @param rawRequest Accumulated raw data from socket
 * @return true if request is complete (headers + body if any)
 * @return false if more data is needed
 *
 * @note Sets _parsedBytes to track consumed bytes for pipelining
 */
bool HttpRequest::parse(const std::string &rawRequest) {
  _parsedBytes = 0;

  // Stage 1: Parse headers
  if (!_headersComplete) {
    if (!parseHeaders(rawRequest))
      return false;
  }

  // If malformed, stop parsing
  if (_isMalformed)
    return true;

  // Stage 2: Parse body if needed
  if (_headersComplete && (_contentLength > 0 || _isChunked)) {
    if (!parseBody(rawRequest))
      return false;
  }

  return true;
}

/**
 * @brief Checks if the request is malformed
 *
 * @return true if parsing detected a protocol error
 */
bool HttpRequest::isMalformed() const { return _isMalformed; }

/**
 * @brief Parses HTTP headers from raw request data
 *
 * Looks for \r\n\r\n to mark end of headers, then parses:
 * - Request line: METHOD PATH?QUERY HTTP/VERSION
 * - Headers: Key: Value pairs
 * - Detects Content-Length and Transfer-Encoding: chunked
 * - Sets keep-alive based on HTTP version and Connection header
 * - Validates Host header for HTTP/1.1
 *
 * @param rawRequest Raw request data
 * @return true if headers are complete and parsed
 * @return false if \r\n\r\n not yet received
 */
bool HttpRequest::parseHeaders(const std::string &rawRequest) {
  // Look for end of headers
  size_t headerEnd = rawRequest.find("\r\n\r\n");
  if (headerEnd == std::string::npos)
    return false;

  _headersComplete = true;
  _parsedBytes = headerEnd + 4;

  std::string headerPart = rawRequest.substr(0, headerEnd);
  std::istringstream ss(headerPart);
  std::string line;

  // Parse request line: METHOD-TARGET-VERSION
  if (!std::getline(ss, line))
    return false;

  std::istringstream firstLine(line);
  std::string fullTarget;
  std::string extra;
  if (!(firstLine >> _method >> fullTarget >> _version) ||
      (firstLine >> extra)) {
    std::cout << "[Debug] Malformed request line: " << line << std::endl;
    _isMalformed = true;
    return true;
  }

  // Separate PATH and QUERY STRING
  size_t qpos = fullTarget.find('?');
  if (qpos != std::string::npos) {
    _path = _urlDecode(fullTarget.substr(0, qpos), false);
    _query = _urlDecode(fullTarget.substr(qpos + 1), true);
  } else {
    _path = _urlDecode(fullTarget, false);
    _query.clear();
  }

  // Set keep-alive default based on HTTP version
  _keepAlive = (_version == "HTTP/1.1");

  // Parse remaining headers
  while (std::getline(ss, line)) {
    if (line == "\r" || line.empty())
      break;

    size_t pos = line.find(":");
    if (pos == std::string::npos)
      continue;

    std::string key = line.substr(0, pos);
    std::string val = line.substr(pos + 1);

    // Trim leading space and trailing \r
    if (!val.empty() && val[0] == ' ')
      val.erase(0, 1);
    if (!val.empty() && val[val.length() - 1] == '\r')
      val.erase(val.length() - 1);

    // Normalize key to lowercase for case-insensitive lookup
    for (size_t i = 0; i < key.length(); ++i) {
      if (key[i] >= 'A' && key[i] <= 'Z')
        key[i] = key[i] - 'A' + 'a';
    }

    _headers[key] = val;

    // Detect Content-Length and Transfer-Encoding
    if (strcasecmp(key.c_str(), "content-length") == 0)
      _contentLength = atoi(val.c_str());
    else if (strcasecmp(key.c_str(), "transfer-encoding") == 0 &&
             val.find("chunked") != std::string::npos)
      _isChunked = true;

    // Handle Connection header override
    if (strcasecmp(key.c_str(), "connection") == 0) {
      if (strcasecmp(val.c_str(), "close") == 0)
        _keepAlive = false;
      else if (strcasecmp(val.c_str(), "keep-alive") == 0)
        _keepAlive = true;
    }
  }

  // Validate: Host header is mandatory in HTTP/1.1
  if (_version == "HTTP/1.1" && _headers.find("host") == _headers.end()) {
    std::cout << "[Debug] HTTP/1.1 request missing Host header" << std::endl;
    _isMalformed = true;
  }

  _parseCookies();
  return true;
}

/**
 * @brief Converts hex character to integer value
 *
 * @param c Hex character (0-9, A-F, a-f)
 * @return Integer value (0-15) or -1 if invalid
 */
static int hexVal(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  return -1;
}

/**
 * @brief Decodes URL-encoded string
 *
 * Converts %XX sequences to their byte values.
 * Optionally converts + to space (for query strings).
 *
 * @param encoded URL-encoded string
 * @param plusAsSpace If true, convert + to space (for query strings)
 * @return Decoded string
 *
 * @note PATH uses %20 for spaces, QUERY uses + for spaces
 */
std::string HttpRequest::_urlDecode(const std::string &encoded,
                                    bool plusAsSpace) const {
  std::string decoded;
  decoded.reserve(encoded.size());

  for (size_t i = 0; i < encoded.size(); ++i) {
    char c = encoded[i];
    if (c == '%' && i + 2 < encoded.size()) {
      int highNibble = hexVal(encoded[i + 1]);
      int lowNibble = hexVal(encoded[i + 2]);
      if (highNibble >= 0 && lowNibble >= 0) {
        decoded.push_back(static_cast<char>((highNibble << 4) | lowNibble));
        i += 2;
      } else {
        // Malformed sequence: keep % literal
        decoded.push_back('%');
      }
    } else if (c == '+' && plusAsSpace) {
      decoded.push_back(' ');
    } else {
      decoded.push_back(c);
    }
  }
  return decoded;
}

/**
 * @brief Parses the request body
 *
 * Handles both Content-Length and chunked transfer encoding.
 *
 * @param rawRequest Raw request data
 * @return true if body is complete
 * @return false if more data needed
 *
 * @note Body size limits are checked in RequestHandler
 */
bool HttpRequest::parseBody(const std::string &rawRequest) {
  // Find body start (after \r\n\r\n)
  size_t bodyStart = rawRequest.find("\r\n\r\n");
  if (bodyStart == std::string::npos)
    return false;
  bodyStart += 4;

  if (_isChunked) {
    std::string chunkedData = rawRequest.substr(bodyStart);
    return parseChunkedBody(chunkedData);
  }

  // Content-Length: wait for all bytes
  size_t bodyBytes = rawRequest.size() - bodyStart;
  if (bodyBytes < static_cast<size_t>(_contentLength))
    return false;

  _body = rawRequest.substr(bodyStart, _contentLength);
  _parsedBytes += _contentLength;
  return true;
}

// ==================== GETTERS ====================

bool HttpRequest::isKeepAlive() const { return _keepAlive; }

const std::string &HttpRequest::getMethod() const { return _method; }

const std::string &HttpRequest::getPath() const { return _path; }

const std::string &HttpRequest::getQuery() const { return _query; }

const std::string &HttpRequest::getVersion() const { return _version; }

const std::string &HttpRequest::getBody() const { return _body; }

const std::map<std::string, std::string> &HttpRequest::getHeaders() const {
  return _headers;
}

bool HttpRequest::headersComplete() const { return _headersComplete; }

bool HttpRequest::isChunked() const { return _isChunked; }

int HttpRequest::getContentLength() const { return _contentLength; }

int HttpRequest::getParsedBytes() const { return _parsedBytes; }

const std::map<std::string, std::string> &HttpRequest::getCookies() const {
  return _cookies;
}

/**
 * @brief Resets the request object for reuse (keep-alive)
 *
 * Clears all parsed data to prepare for next request.
 */
void HttpRequest::reset() {
  _headersComplete = false;
  _isChunked = false;
  _keepAlive = false;
  _isMalformed = false;
  _parsedBytes = 0;
  _contentLength = -1;
  _method.clear();
  _path.clear();
  _query.clear();
  _version.clear();
  _headers.clear();
  _cookies.clear();
  _body.clear();
}

/**
 * @brief Gets a single header value by key (case-insensitive)
 *
 * @param key Header name
 * @return Header value, or empty string if not found
 */
std::string HttpRequest::getOneHeader(const std::string &key) const {
  std::string lowerKey = key;
  for (size_t i = 0; i < lowerKey.length(); ++i) {
    if (lowerKey[i] >= 'A' && lowerKey[i] <= 'Z')
      lowerKey[i] = lowerKey[i] - 'A' + 'a';
  }
  std::map<std::string, std::string>::const_iterator it =
      _headers.find(lowerKey);
  if (it != _headers.end()) {
    return it->second;
  }
  return "";
}

/**
 * @brief Parses chunked transfer encoding body
 *
 * Format: <size_hex>\r\n<data>\r\n...<0>\r\n\r\n
 *
 * @param chunkedData Raw data after headers
 * @return true if complete (found 0\r\n terminator)
 * @return false if more data needed
 */
bool HttpRequest::parseChunkedBody(const std::string &chunkedData) {
  std::string result;
  size_t pos = 0;

  while (pos < chunkedData.size()) {
    // Find end of size line
    size_t lineEnd = chunkedData.find("\r\n", pos);
    if (lineEnd == std::string::npos) {
      return false;
    }

    // Extract chunk size (hex)
    std::string chunkSizeStr = chunkedData.substr(pos, lineEnd - pos);

    // Ignore chunk extensions after ';'
    size_t semicolon = chunkSizeStr.find(';');
    if (semicolon != std::string::npos) {
      chunkSizeStr = chunkSizeStr.substr(0, semicolon);
    }

    // Convert hex to integer
    char *endPtr;
    long chunkSize = std::strtol(chunkSizeStr.c_str(), &endPtr, 16);

    if (endPtr == chunkSizeStr.c_str() || chunkSize < 0) {
      std::cerr << "❌ [Error] Chunked: invalid size '" << chunkSizeStr << "'\n";
      return false;
    }

    // Final chunk (size 0) marks end of body
    if (chunkSize == 0) {
      _body = result;
      _parsedBytes = lineEnd + 4;
      return true;
    }

    // Verify we have enough data
    size_t dataStart = lineEnd + 2;
    size_t chunkLen = static_cast<size_t>(chunkSize);

    if (dataStart + chunkLen + 2 > chunkedData.size()) {
      return false;
    }

    // Extract chunk data
    result.append(chunkedData, dataStart, chunkLen);

    // Move to next chunk
    pos = dataStart + chunkLen + 2;
  }

  return false;
}

/**
 * @brief Parses Cookie header into key-value pairs
 *
 * Populates _cookies map from Cookie header.
 * Format: name1=value1; name2=value2
 */
void HttpRequest::_parseCookies() {
  _cookies.clear();
  std::map<std::string, std::string>::const_iterator it =
      _headers.find("cookie");
  if (it == _headers.end())
    return;

  std::string cookieHeader = it->second;
  std::istringstream ss(cookieHeader);
  std::string item;
  while (std::getline(ss, item, ';')) {
    // Trim leading spaces
    size_t start = item.find_first_not_of(" ");
    if (start == std::string::npos)
      continue;
    item = item.substr(start);

    size_t eq = item.find('=');
    if (eq != std::string::npos) {
      std::string key = item.substr(0, eq);
      std::string val = item.substr(eq + 1);
      _cookies[key] = val;
    }
  }
}
