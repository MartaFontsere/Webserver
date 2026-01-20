#include "../../includes/cgi/CGIOutputParser.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

/**
 * @file CGIOutputParser.cpp
 * @brief CGI output parsing - separates HTTP headers from body (RFC 3875
 * compliant)
 *
 * This module parses the raw output from CGI scripts, which follows the CGI
 * response format defined in RFC 3875. CGI scripts output HTTP-like
 * responses with headers and body separated by a blank line (\r\n\r\n).
 *
 * CGI output format (RFC 3875 section 6):
 *   Header-Name: Header-Value\r\n
 *   Another-Header: Value\r\n
 *   Status: 200 OK\r\n
 *   \r\n                          ← Blank line separator
 *   <body content starts here>
 *
 * Parsing responsibilities:
 * - Split headers section from body section
 * - Parse individual header lines into key-value pairs
 * - Extract HTTP status code from "Status" header (or default to 200)
 * - Clean carriage returns (\r) from line endings
 * - Store headers in map for easy access
 *
 * Status code handling:
 * CGI scripts can specify HTTP status via "Status" header:
 *   Status: 404 Not Found       → Extract 404
 *   Status: 302 Found           → Extract 302
 *   (no Status header)          → Default 200 OK
 *
 * Key design decisions:
 * - Uses istringstream for line-by-line parsing (C++98 idiomatic)
 * - Manually cleans \r (getline may leave it on some platforms)
 * - const_iterator in getters (const correctness)
 * - Default status 200 matches RFC 3875 specification
 *
 * @note Compatible with output from PHP-CGI, Python CGI, Bash CGI, etc.
 * @see RFC 3875 section 6 (CGI Response)
 */

/**
 * @brief Default constructor
 *
 * Initializes a CGIOutputParser with default values:
 * - _statusCode = 0 (will be set to 200 or parsed value in parse())
 * - _headers = {} (empty map, filled by parse())
 * - _body = "" (empty string, filled by parse())
 *
 * The object is not usable until parse() is called with raw CGI output.
 *
 * @note _statusCode is initialized to 0 as sentinel value (invalid HTTP
 * code)
 */
CGIOutputParser::CGIOutputParser() { _statusCode = 0; }

/**
 * @brief Destructor
 *
 * Cleans up the CGIOutputParser object. No explicit cleanup needed because:
 * - _headers is std::map (automatically destroyed)
 * - _body is std::string (automatically destroyed)
 * - _statusCode is primitive int (no cleanup needed)
 *
 * @note Destructor is empty due to RAII principle (automatic resource
 * management)
 */
CGIOutputParser::~CGIOutputParser() {}

/**
 * @brief Parses raw CGI output into headers and body
 *
 * Splits the raw output from a CGI script into two sections using the standard
 * HTTP separator ("\r\n\r\n") and parses headers into a key-value map.
 *
 * Parsing algorithm:
 *
 * STEP 1: Split headers from body
 *   - Find "\r\n\r\n" (double CRLF = blank line)
 *   - Everything before → headers section
 *   - Everything after (+4 to skip separator) → body section
 *
 * STEP 2: Parse headers line by line
 *   - Use istringstream to iterate through header lines
 *   - For each line:
 *     a. Remove trailing \r if present (getline inconsistency)
 *     b. Find ':' position (separator between key and value)
 *     c. Extract key (before ':')
 *     d. Extract value (after ': ' - note the space)
 *     e. Store in _headers map
 *
 * Header format (RFC 3875):
 *   Content-Type: text/html\r\n
 *   Status: 200 OK\r\n
 *   X-Custom-Header: value\r\n
 *
 * Parsing example:
 *   Input line: "Content-Type: text/html\r\n"
 *   After getline: "Content-Type: text/html\r"  (may have \r)
 *   After erase: "Content-Type: text/html"
 *   colonPos = 12
 *   key = "Content-Type" (substr 0 to 12)
 *   value = "text/html" (substr 14 to end, skipping ": ")
 *
 * Why +2 in value extraction:
 *   line.substr(colonPos + 2)
 *              \_________/
 *                   +1 for ':'
 *                   +1 for ' ' (space after colon)
 *
 * Complete example:
 *   Raw output:
 *     "Content-Type: text/html\r\n"
 *     "Status: 404 Not Found\r\n"
 *     "\r\n"
 *     "<html><body>Not Found</body></html>"
 *
 *   After parsing:
 *     _headers = {
 *       "Content-Type": "text/html",
 *       "Status": "404 Not Found"
 *     }
 *     _body = "<html><body>Not Found</body></html>"
 *
 * @param rawOutput Complete output from CGI script (headers + body)
 *
 * @note Assumes rawOutput contains "\r\n\r\n" separator (standard CGI format)
 * @note Trailing \r cleanup is necessary because getline behavior varies by
 * platform
 * @warning Does not validate header format - assumes well-formed CGI output
 */
void CGIOutputParser::parse(const std::string &rawOutput)
{
  // STEP 1: Split headers from body using double CRLF separator
  size_t pos = rawOutput.find("\r\n\r\n");
  if (pos == std::string::npos)
  {
    // Fallback to \n\n if \r\n\r\n is not found
    pos = rawOutput.find("\n\n");
    if (pos == std::string::npos)
    {
      _body = rawOutput;
      return;
    }
    _body = rawOutput.substr(pos + 2);
  }
  else
  {
    _body = rawOutput.substr(pos + 4);
  }
  std::string headersSection = rawOutput.substr(0, pos);
  // STEP 2: Parse headers line by line
  std::istringstream stream(headersSection);
  std::string line;

  while (std::getline(stream, line))
  {

    if (line[line.size() - 1] == '\r')
      line.erase(line.size() - 1, 1);

    size_t colonPos = line.find(":");
    if (colonPos == std::string::npos)
      continue;
    std::string keyLine = line.substr(0, colonPos);
    std::string valueLine = line.substr(colonPos + 1);

    size_t firstNotSpace = valueLine.find_first_not_of(" \t");
    if (firstNotSpace != std::string::npos)
      valueLine = valueLine.substr(firstNotSpace);

    if (toUpperCase(keyLine) == "SET-COOKIE")
    {
      _setCookies.push_back(valueLine);
    }
    else
    {
      _headers[keyLine] = valueLine;
    }
  }
}

/**
 * @brief Extracts HTTP status code from "Status" header
 *
 * Searches for the "Status" header in parsed headers and extracts the numeric
 * status code (first 3 characters). Returns 200 (OK) if no Status header
 * present.
 *
 * Status header format (RFC 3875 section 6.3.3):
 *   Status: nnn reason-phrase
 *
 *   Examples:
 *     Status: 200 OK
 *     Status: 404 Not Found
 *     Status: 302 Found
 *     Status: 500 Internal Server Error
 *
 * Extraction process:
 * 1. Search for "Status" key in _headers map (using find for const safety)
 * 2. If found:
 *    a. Extract first 3 characters (status code)
 *    b. Convert to int using stringstream
 *    c. Return code
 * 3. If not found: return 200 (default per RFC 3875)
 *
 * Examples:
 *   _headers["Status"] = "404 Not Found"
 *     → statusValue = "404 Not Found"
 *     → codeStr = "404"
 *     → code = 404
 *
 *   _headers["Status"] = "200 OK"
 *     → code = 200
 *
 *   (no "Status" header)
 *     → code = 200 (default)
 *
 * Why const_iterator:
 * - Method is const (cannot modify _headers)
 * - operator[] would be non-const (inserts key if missing)
 * - find() returns const_iterator when called on const object
 *
 * RFC 3875 default behavior:
 * "If a script does not specify a Status header, 200 OK is assumed."
 *
 * @return HTTP status code (200, 404, 500, etc.)
 *
 * @note Always returns 200 if no "Status" header present (RFC 3875 compliant)
 * @note Uses stringstream for string-to-int conversion (C++98 compatible)
 */
int CGIOutputParser::getStatusCode() const
{
  std::map<std::string, std::string>::const_iterator it =
      _headers.find("Status");
  if (it != _headers.end())
  {
    std::string statusValue = it->second;
    std::string codeStr = statusValue.substr(0, 3);
    std::stringstream ss(codeStr);
    int code;
    ss >> code;

    return code;
  }
  return 200; // Default status per RFC 3875
}

/**
 * @brief Returns all parsed HTTP headers
 *
 * Provides access to the complete map of headers extracted from the CGI output.
 * Headers are stored in their original format (key capitalization preserved).
 *
 * Typical headers returned:
 *   {
 *     "Content-Type": "text/html",
 *     "Status": "200 OK",
 *     "Set-Cookie": "sessionid=abc123",
 *     "X-Custom-Header": "custom-value"
 *   }
 *
 * @return Map of header names to header values
 *
 * @note Returns copy of _headers (not reference) - caller can modify safely
 * @note Header names are case-sensitive as stored by CGI script
 */
std::map<std::string, std::string> CGIOutputParser::getHeaders() const
{
  return _headers;
}

std::vector<std::string> CGIOutputParser::getSetCookies() const
{
  return _setCookies;
}

/**
 * @brief Returns the parsed body content
 *
 * Provides access to the complete body section of the CGI output. This is
 * everything after the "\r\n\r\n" separator.
 *
 * Body content can be:
 * - HTML page: "<html><body>...</body></html>"
 * - JSON response: '{"status":"ok","data":[...]}'
 * - Plain text: "Hello World"
 * - Binary data: Image/PDF bytes (though less common in CGI)
 *
 * @return Complete body content as string
 *
 * @note Returns copy of _body (not reference) - caller can modify safely
 * @note Body may be empty if script outputs only headers
 */
std::string CGIOutputParser::getBody() const
{
  return _body;
}