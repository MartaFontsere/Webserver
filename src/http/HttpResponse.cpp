#include "http/HttpResponse.hpp"
#include <ctime>
#include <sstream>

/**
 * @file HttpResponse.cpp
 * @brief HTTP response construction and serialization
 *
 * This module handles the creation and formatting of HTTP responses.
 * It provides methods to:
 * - Set status code and message
 * - Add headers and cookies
 * - Set response body
 * - Generate built-in error pages with modern styling
 * - Build the final response string for sending
 *
 * Response structure follows RFC 9110:
 * ```
 * HTTP/1.1 200 OK\r\n
 * Server: webserv/1.0\r\n
 * Date: Mon, 01 Jan 2024 12:00:00 GMT\r\n
 * Content-Type: text/html\r\n
 * Content-Length: 42\r\n
 * \r\n
 * <body content>
 * ```
 *
 * @see RequestHandler for response generation
 * @see StaticFileHandler for file serving
 */

// ==================== CONSTRUCTORS ====================

/**
 * @brief Default constructor
 *
 * Initializes a response with 200 OK status and HTTP/1.1 version.
 */
HttpResponse::HttpResponse()
    : _statusCode(200), _statusMessage("OK"), _httpVersion("HTTP/1.1"),
      _cgiPending(false) {}

/**
 * @brief Destructor
 */
HttpResponse::~HttpResponse() {}

// ==================== STATIC HELPERS ====================

/**
 * @brief Generates current date-time in HTTP format (RFC 9110)
 *
 * Format: "Day, DD Mon YYYY HH:MM:SS GMT"
 * Example: "Mon, 15 Jan 2024 14:30:00 GMT"
 *
 * @return Date string in HTTP format
 *
 * @note Uses gmtime (UTC/GMT) as required by RFC
 * @note The Date header is mandatory in HTTP responses
 */
static std::string getHttpDate() {
  time_t currentTime;
  time(&currentTime);
  struct tm *timeInfo = gmtime(&currentTime);
  char buffer[80];
  strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
  return std::string(buffer);
}

/**
 * @brief Maps HTTP status code to standard reason phrase
 *
 * @param code HTTP status code (e.g., 200, 404, 500)
 * @return Standard reason phrase for the code
 *
 * @note Public static method - can be called from other modules (e.g.,
 * CGIHandler)
 */
std::string HttpResponse::getHttpStatusMessage(int code) {
  switch (code) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Request Entity Too Large";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  default:
    return "Internal Server Error";
  }
}

// ==================== SETTERS ====================

/**
 * @brief Sets the HTTP status code and message
 *
 * @param code HTTP status code (e.g., 200, 404)
 * @param message Status message (e.g., "OK", "Not Found")
 */
void HttpResponse::setStatus(int code, const std::string &message) {
  _statusCode = code;
  _statusMessage = message;
}

/**
 * @brief Sets or updates a response header
 *
 * @param key Header name (e.g., "Content-Type")
 * @param value Header value (e.g., "text/html")
 */
void HttpResponse::setHeader(const std::string &key, const std::string &value) {
  _headers[key] = value;
}

/**
 * @brief Adds a Set-Cookie header
 *
 * @param cookie Complete cookie string (e.g., "session=abc123; HttpOnly")
 *
 * @note Multiple cookies can be added; each generates a separate Set-Cookie
 * header
 */
void HttpResponse::setCookie(const std::string &cookie) {
  _setCookies.push_back(cookie);
}

/**
 * @brief Sets the response body and updates Content-Length
 *
 * @param body Response body content
 *
 * @note Automatically calculates and sets Content-Length header
 */
void HttpResponse::setBody(const std::string &body) {
  _body = body;
  std::ostringstream oss;
  oss << _body.size();
  _headers["Content-Length"] = oss.str();
}

/**
 * @brief Returns the current status code
 *
 * @return HTTP status code
 */
int HttpResponse::getStatusCode() const { return _statusCode; }

/**
 * @brief Sets the CGI pending flag
 *
 * @param pending true if response is waiting for CGI to complete
 */
void HttpResponse::setCGIPending(bool pending) { _cgiPending = pending; }

/**
 * @brief Checks if response is waiting for CGI
 *
 * @return true if CGI execution is pending
 */
bool HttpResponse::isCGIPending() const { return _cgiPending; }

// ==================== ERROR HANDLING ====================

/**
 * @brief Generates a styled error page for the given status code
 *
 * Creates a modern, dark-themed HTML error page with:
 * - Gradient background
 * - Card-style layout
 * - Appropriate icon and message
 * - Back to dashboard link
 *
 * @param code HTTP error code (400, 403, 404, 405, 413, 500)
 *
 * @note Sets Content-Type to text/html
 * @note Sets X-Content-Type-Options: nosniff for security
 */
void HttpResponse::setErrorResponse(int code) {
  _httpVersion = "HTTP/1.1";
  _statusCode = code;
  _statusMessage = getHttpStatusMessage(code);

  // Common CSS for all error pages (dark theme)
  std::string css =
      "<style>"
      "*{box-sizing:border-box;margin:0;padding:0}"
      "body{font-family:'Segoe UI',system-ui,sans-serif;"
      "background:linear-gradient(135deg,#0f172a 0%,#1e293b 100%);"
      "color:#f8fafc;min-height:100vh;display:flex;align-items:center;"
      "justify-content:center;padding:2rem}"
      ".card{background:rgba(30,41,59,0.8);border-radius:1rem;padding:3rem;"
      "text-align:center;box-shadow:0 25px 50px -12px rgba(0,0,0,0.5);"
      "max-width:500px}"
      ".icon{font-size:4rem;margin-bottom:1rem}"
      "h1{color:#f87171;font-size:1.8rem;margin-bottom:0.5rem}"
      "p{color:#94a3b8;margin-top:1rem}"
      "a{color:#38bdf8;text-decoration:none}"
      "a:hover{text-decoration:underline}"
      ".code{font-size:5rem;font-weight:700;color:#38bdf8;opacity:0.3}"
      "</style>";

  std::string head = "<html lang=\"en\"><head><meta charset=\"UTF-8\">"
                     "<meta name=\"viewport\" content=\"width=device-width,"
                     "initial-scale=1.0\">" +
                     css + "</head><body><div class=\"card\">";
  std::string foot =
      "<p><a href=\"/tests/\">← Back to Dashboard</a></p></div></body></html>";

  switch (code) {
  case 400:
    _body = head +
            "<div class=\"code\">400</div>"
            "<div class=\"icon\">🚫</div>"
            "<h1>Bad Request</h1>"
            "<p>The server could not understand your request.</p>" +
            foot;
    break;
  case 403:
    _body = head +
            "<div class=\"code\">403</div>"
            "<div class=\"icon\">🔒</div>"
            "<h1>Forbidden</h1>"
            "<p>You don't have permission to access this resource.</p>" +
            foot;
    break;
  case 404:
    _body = head +
            "<div class=\"code\">404</div>"
            "<div class=\"icon\">🔍</div>"
            "<h1>Not Found</h1>"
            "<p>The page you're looking for doesn't exist.</p>" +
            foot;
    break;
  case 405:
    _body = head +
            "<div class=\"code\">405</div>"
            "<div class=\"icon\">⛔</div>"
            "<h1>Method Not Allowed</h1>"
            "<p>This HTTP method is not allowed for this resource.</p>" +
            foot;
    break;
  case 413:
    _body = head +
            "<div class=\"code\">413</div>"
            "<div class=\"icon\">📦</div>"
            "<h1>Payload Too Large</h1>"
            "<p>The uploaded file exceeds the maximum size limit (10MB).</p>" +
            foot;
    break;
  case 501:
    _body = head +
            "<div class=\"code\">501</div>"
            "<div class=\"icon\">🚧</div>"
            "<h1>Not Implemented</h1>"
            "<p>This feature is not supported by the server.</p>" +
            foot;
    break;
  case 500:
  default:
    _body = head +
            "<div class=\"code\">500</div>"
            "<div class=\"icon\">💥</div>"
            "<h1>Internal Server Error</h1>"
            "<p>Something went wrong on our end. Please try again later.</p>" +
            foot;
    break;
  }

  _headers["Content-Type"] = "text/html";
  _headers["X-Content-Type-Options"] = "nosniff";
  std::ostringstream length;
  length << _body.size();
  _headers["Content-Length"] = length.str();
}

// ==================== RESPONSE BUILDER ====================

/**
 * @brief Builds the complete HTTP response string
 *
 * Constructs the response in this order:
 * 1. Status line: "HTTP/1.1 200 OK\r\n"
 * 2. Server header: "Server: webserv/1.0\r\n"
 * 3. Date header: RFC-formatted timestamp
 * 4. User-set headers from _headers map
 * 5. Content-Length (if not already set)
 * 6. Set-Cookie headers (if any)
 * 7. Blank line separator
 * 8. Body content
 *
 * @return Complete HTTP response ready to send
 *
 * @note Headers are output in alphabetical order (std::map behavior)
 */
std::string HttpResponse::buildResponse() const {
  std::ostringstream oss;

  // Step 1: Status line
  oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

  // Step 2-3: Automatic headers (RFC-compliant)
  oss << "Server: webserv/1.0\r\n";
  oss << "Date: " << getHttpDate() << "\r\n";

  // Step 4: User-set headers
  for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
       it != _headers.end(); ++it) {
    oss << it->first << ": " << it->second << "\r\n";
  }

  // Step 5: Automatic Content-Length if not manually set
  if (_headers.find("Content-Length") == _headers.end()) {
    oss << "Content-Length: " << _body.size() << "\r\n";
  }

  // Step 6: Set-Cookie headers
  for (std::vector<std::string>::const_iterator it = _setCookies.begin();
       it != _setCookies.end(); ++it) {
    oss << "Set-Cookie: " << *it << "\r\n";
  }

  // Step 7: Mandatory blank line separating headers from body
  oss << "\r\n";

  // Step 8: Body
  oss << _body;

  return oss.str();
}