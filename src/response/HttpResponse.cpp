#include "../../includes/response/HttpResponse.hpp"
#include "../../includes/response/UtilsResponse.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

/**
 * @file HttpResponse.cpp
 * @brief HTTP/1.1 response builder implementation - RFC 9110 compliant
 *
 * Implements complete HTTP response construction with automatic headers,
 * MIME type detection, and flexible error page management.
 *
 * Key features implemented:
 * - Automatic RFC-compliant headers (Server, Date, Content-Length)
 * - MIME type auto-detection for 14 file extensions
 * - Error page system with file loading and hardcoded fallback
 * - Orthodox Canonical Form (OCF) compliance
 *
 * Response lifecycle:
 * 1. Create HttpResponse object (constructor)
 * 2. Set status (setStatus) or error (setErrorResponse)
 * 3. Optionally set custom headers (setHeader)
 * 4. Set body content (setBody or auto from error page)
 * 5. Build final HTTP string (buildResponse) - adds automatic headers
 * 6. Send to socket (handled by Server class)
 *
 * Automatic headers added in buildResponse():
 * - Server: webserv/1.0 (identifies server software)
 * - Date: Current GMT timestamp (RFC 9110 required)
 * - Content-Length: Body size in bytes (if body exists)
 *
 * MIME type support:
 * HTML, CSS, JavaScript, Images (JPEG, PNG, GIF, SVG),
 * Scripts (PHP, Python, Shell), Data (JSON, TXT)
 *
 * Error page fallback chain:
 * 1. Try loading from LocationConfig errorPages map
 * 2. If file read fails → hardcoded HTML
 * 3. If code unknown → 500 Internal Server Error
 *
 * @see UtilsResponse for getHttpDate() and sizeToString() helpers
 */

/**
 * @brief Default constructor - Initializes response with 200 OK defaults
 *
 * Creates an HttpResponse ready for success responses. All containers
 * (STL map for headers, string for body) initialize empty automatically.
 *
 * Default values:
 * - _statusCode = 200 (OK - most common response)
 * - _statusMessage = "OK"
 * - _httpVersion = "HTTP/1.1" (fixed, per project requirement)
 * - _headers = {} (empty map, filled by setters or buildResponse)
 * - _body = "" (empty, set by setBody or setErrorResponse)
 *
 * Initialization list only includes primitives because:
 * - STL containers (map, string) auto-initialize to empty
 * - Explicit initialization avoids garbage values
 *
 * Usage example:
 *   HttpResponse response;           // 200 OK by default
 *   response.setBody("<html>...");
 *   std::string http = response.buildResponse();
 *
 * @note Headers added automatically in buildResponse(), not here
 * @note Date header calculated at build time for accuracy
 */
HttpResponse::HttpResponse()
    : _statusCode(200),
      _statusMessage("OK"),
      _httpVersion("HTTP/1.1")
{
}

/**
 * @brief Copy constructor - Deep copies all response data
 *
 * Creates independent copy of an existing HttpResponse. All members
 * are STL containers or primitives, so default copy is safe (no raw pointers).
 *
 * Copy behavior:
 * - int, string: value copy
 * - map<string, string>: deep copy of all headers
 * - All copies independent (modifying copy doesn't affect original)
 *
 * Usage example:
 *   HttpResponse resp1;
 *   resp1.setStatus(404, "Not Found");
 *   HttpResponse resp2(resp1);  // Independent copy
 *   resp2.setStatus(200, "OK"); // Doesn't affect resp1
 *
 * @param other HttpResponse to copy from
 *
 * @note All 5 members copied in initialization list
 * @note STL containers perform deep copy automatically
 */
HttpResponse::HttpResponse(const HttpResponse &other)
    : _statusCode(other._statusCode),
      _statusMessage(other._statusMessage),
      _httpVersion(other._httpVersion),
      _headers(other._headers),
      _body(other._body)
{
}

/**
 * @brief Assignment operator - Copies response data from another object
 *
 * Overwrites current response with values from another HttpResponse.
 * Self-assignment protection prevents bugs when resp = resp.
 *
 * Assignment behavior:
 * - All 5 members assigned individually
 * - STL containers handle deep copy safely
 * - Primitives copied by value
 *
 * Usage example:
 *   HttpResponse resp1, resp2;
 *   resp1.setStatus(200, "OK");
 *   resp2 = resp1;              // Copy all data
 *   resp2.setStatus(404, "Not Found"); // Independent
 *
 * @param other HttpResponse to copy from
 * @return Reference to this object (*this) for chained assignments (a = b = c)
 *
 * @note Self-assignment check required by OCF
 * @note Returns *this to enable assignment chaining
 */
HttpResponse &HttpResponse::operator=(const HttpResponse &other)
{
    if (this != &other)
    {
        _statusCode = other._statusCode;
        _statusMessage = other._statusMessage;
        _httpVersion = other._httpVersion;
        _headers = other._headers;
        _body = other._body;
    }
    return *this;
}

/**
 * @brief Destructor - Automatic cleanup via RAII
 *
 * Destroys HttpResponse object. No manual cleanup needed because:
 * - All members are STL containers (std::map, std::string) or primitives
 * - STL destructors handle memory release automatically
 *
 * @note Empty body because STL handles all cleanup (RAII principle)
 * @note No manual delete/free needed
 */
HttpResponse::~HttpResponse()
{
}

// ==================== PRIVATE HELPERS ====================

/**
 * @brief Detects MIME type from file extension
 *
 * Maps file extension to appropriate Content-Type header value.
 * Supports 14 common web file types with fallback for unknown.
 *
 * Supported extensions (14 total):
 * - HTML: .html, .htm → text/html
 * - CSS: .css → text/css
 * - JavaScript: .js → application/javascript (RFC 9239)
 * - Images: .jpg/.jpeg → image/jpeg, .png, .gif, .svg
 * - Scripts: .php, .py, .sh (for source code serving)
 * - Data: .txt, .json
 *
 * Algorithm:
 * 1. Find last '.' in path
 * 2. Extract extension (includes dot: ".html")
 * 3. Look up in map
 * 4. Return MIME type or default "application/octet-stream"
 *
 * Default MIME type rationale:
 * application/octet-stream = generic binary stream
 * - Safe for unknown file types
 * - Browser will download instead of trying to display
 *
 * Usage example:
 *   getMimeType("/path/style.css")    → "text/css"
 *   getMimeType("image.jpg")          → "image/jpeg"
 *   getMimeType("unknown.xyz")        → "application/octet-stream"
 *   getMimeType("no_extension")       → "application/octet-stream"
 *
 * @param path File path or name with extension
 * @return MIME type string for Content-Type header
 *
 * @note Extension comparison includes dot (".html" not "html")
 * @note Map created on each call (optimize with static if performance issue)
 * @note Case-sensitive (".HTML" won't match)
 */
std::string HttpResponse::getMimeType(const std::string &path) const
{
    size_t lastDot = path.find_last_of(".");
    if (lastDot != std::string::npos)
    {
        std::map<std::string, std::string> mimeTypes;
        mimeTypes[".html"] = "text/html";
        mimeTypes[".htm"] = "text/html";
        mimeTypes[".css"] = "text/css";
        mimeTypes[".txt"] = "text/plain";
        mimeTypes[".jpg"] = "image/jpeg";
        mimeTypes[".jpeg"] = "image/jpeg";
        mimeTypes[".png"] = "image/png";
        mimeTypes[".gif"] = "image/gif";
        mimeTypes[".svg"] = "image/svg+xml";
        mimeTypes[".php"] = "text/x-php";
        mimeTypes[".py"] = "text/x-python";
        mimeTypes[".sh"] = "application/x-sh";
        mimeTypes[".json"] = "application/json";
        mimeTypes[".js"] = "application/javascript";
        std::map<std::string, std::string>::const_iterator it = mimeTypes.find(path.substr(lastDot));
        if (it != mimeTypes.end())
            return it->second;
        else
            return "application/octet-stream";
    }
    return "application/octet-stream";
}

/**
 * @brief Maps HTTP status code to standard reason phrase
 *
 * Returns RFC-compliant status message for supported error codes.
 * Used by both error response methods to maintain consistency.
 *
 * Supported codes (5 total):
 * - 403: Forbidden (access denied)
 * - 404: Not Found (resource doesn't exist)
 * - 405: Method Not Allowed (GET on POST-only resource, etc.)
 * - 413: Request Entity Too Large (body exceeds client_max_body_size)
 * - 500: Internal Server Error (server-side error, also default)
 *
 * Default behavior:
 * - Unknown codes → "Internal Server Error"
 * - Rationale: safer to report server error than fabricate message
 *
 * Usage example:
 *   getStatusMessage(404) → "Not Found"
 *   getStatusMessage(999) → "Internal Server Error" (default)
 *
 * @param code HTTP status code (3-digit number)
 * @return Standard reason phrase for status line
 *
 * @note Default is 500 (safer than fake message for unknown codes)
 * @note Easily extensible (add case for new codes)
 */
std::string HttpResponse::getStatusMessage(int code) const
{
    switch (code)
    {
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
    default:
        return "Internal Server Error";
    }
}

/**
 * @brief Reads error page HTML from filesystem
 *
 * Attempts to load custom error page HTML from file path.
 * Used by setErrorResponse(code, map) for custom error pages.
 *
 * Read algorithm:
 * 1. Open file with ifstream (C++98 requires .c_str())
 * 2. Check if opened successfully
 * 3. Read entire file into stringstream buffer
 * 4. Close file
 * 5. Return content as string (or empty if failed)
 *
 * Failure handling:
 * - File not found → Returns empty string + stderr message
 * - Permission denied → Returns empty string + stderr message
 * - Caller checks empty() and falls back to hardcoded HTML
 *
 * Usage example:
 *   std::string html = readErrorFile("/path/error404.html");
 *   if (!html.empty()) {
 *       // Use custom HTML
 *   } else {
 *       // Fallback to hardcoded
 *   }
 *
 * @param path Full file path to error page HTML
 * @return File contents as string, or empty string if read fails
 *
 * @note C++98 requires .c_str() for ifstream constructor
 * @note Reads entire file into memory (fine for HTML error pages <100KB)
 * @note Error message printed to stderr for debugging
 */
std::string HttpResponse::readErrorFile(const std::string &path) const
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        std::cerr << "❌ Error: No se pudo abrir " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

// ==================== PUBLIC SETTERS ====================

/**
 * @brief Auto-detects and sets Content-Type header from file path
 *
 * Convenience method that combines MIME detection and header setting.
 * Extracts extension from path, looks up MIME type, sets Content-Type header.
 *
 * Usage example:
 *   response.setContentTypeFromPath("/www/style.css");
 *   // Sets header: "Content-Type: text/css"
 *
 *   response.setContentTypeFromPath("image.png");
 *   // Sets header: "Content-Type: image/png"
 *
 * @param path File path with extension (used for MIME detection)
 *
 * @note Overwrites existing Content-Type if already set
 * @note Uses getMimeType() internally (supports 14 extensions)
 */
void HttpResponse::setContentTypeFromPath(const std::string &path)
{
    setHeader("Content-Type", getMimeType(path));
}

/**
 * @brief Sets HTTP status code and reason phrase
 *
 * Manually sets status line components. Typically used for success
 * responses or custom error messages.
 *
 * Usage example:
 *   response.setStatus(200, "OK");
 *   response.setStatus(201, "Created");
 *   response.setStatus(301, "Moved Permanently");
 *
 * @param code HTTP status code (100-599)
 * @param message Reason phrase (e.g., "OK", "Not Found")
 *
 * @note For standard errors, prefer setErrorResponse() (auto message + HTML)
 */
void HttpResponse::setStatus(int code, const std::string &message)
{
    _statusCode = code;
    _statusMessage = message;
}

/**
 * @brief Sets or overwrites a response header
 *
 * Adds header to internal map. If key exists, value is overwritten.
 * Headers are serialized in buildResponse().
 *
 * Usage example:
 *   response.setHeader("Content-Type", "text/html");
 *   response.setHeader("Cache-Control", "no-cache");
 *   response.setHeader("Set-Cookie", "session=abc123");
 *
 * @param key Header name (case-sensitive, use standard capitalization)
 * @param value Header value
 *
 * @note Automatic headers (Server, Date, Content-Length) can be overridden
 * @note buildResponse() adds Server, Date, Content-Length automatically
 */
void HttpResponse::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

/**
 * @brief Sets response body content
 *
 * Assigns HTTP response body (HTML, JSON, file contents, etc.).
 * Content-Length calculated automatically in buildResponse().
 *
 * Usage example:
 *   response.setBody("<html><body>Hello</body></html>");
 *   response.setBody("{\"status\":\"ok\"}");
 *   response.setBody(fileContents);  // From file read
 *
 * @param body Response body content (string)
 *
 * @note Content-Length auto-calculated from body.size()
 * @note Overwrites previous body content
 */
void HttpResponse::setBody(const std::string &body)
{
    _body = body;
}

// ==================== ERROR HANDLING ====================

/**
 * @brief Creates error response with hardcoded HTML (fallback version)
 *
 * Generates complete error response with minimal HTML body.
 * Used when no custom error pages configured or as fallback
 * when file loading fails.
 *
 * Sets all required components:
 * - HTTP version: HTTP/1.1
 * - Status code and message (via getStatusMessage)
 * - Minimal HTML body with error code
 * - Content-Type: text/html
 * - Content-Length: calculated from body
 *
 * Hardcoded HTML format:
 *   <html><body><h1>[CODE] [MESSAGE]</h1></body></html>
 *
 * Supported codes: 403, 404, 405, 413, 500
 * Unknown codes → 500 Internal Server Error
 *
 * Usage example:
 *   response.setErrorResponse(404);
 *   // Body: <html><body><h1>404 Not Found</h1></body></html>
 *
 * @param code HTTP error status code (4xx or 5xx)
 *
 * @note Used as fallback by setErrorResponse(code, map) version
 * @note Minimal HTML (no CSS, no images, basic structure)
 */
void HttpResponse::setErrorResponse(int code)
{
    _httpVersion = "HTTP/1.1";
    _statusCode = code;
    _statusMessage = getStatusMessage(code);
    switch (code)
    {
    case 403:
        _body = "<html><body><h1>403 Forbidden</h1></body></html>";
        break;
    case 404:
        _body = "<html><body><h1>404 Not Found</h1></body></html>";
        break;
    case 405:
        _body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        break;
    case 413:
        _body = "<html><body><h1>413 Request Entity Too Large</h1>"
                "<p>Maximum body size is 10MB</p></body></html>";
        break;
    case 500:
    default:
        _body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    }

    _headers["Content-Type"] = "text/html";
    _headers["Content-Length"] = sizeToString(_body.size());
}

/**
 * @brief Creates error response with custom HTML from file (preferred version)
 *
 * Attempts to load custom error page HTML from LocationConfig errorPages map.
 * Falls back to hardcoded HTML if file not found or read fails.
 *
 * Fallback chain:
 * 1. Look up code in errorPages map
 * 2. If found → try reading file with readErrorFile()
 * 3. If file read success → use custom HTML
 * 4. If not found or read fails → call setErrorResponse(code) for hardcoded
 *
 * Custom error page benefits:
 * - Branded design matching website
 * - Better UX (actionable links, search box, etc.)
 * - Consistent styling (CSS, images, etc.)
 *
 * Map structure (from LocationConfig):
 *   {403: "/error_files/error403.html",
 *    404: "/error_files/error404.html",
 *    ...}
 *
 * Usage example:
 *   std::map<int, std::string> pages;
 *   pages[404] = "/www/errors/404.html";
 *   response.setErrorResponse(404, pages);
 *   // Loads /www/errors/404.html or falls back to hardcoded
 *
 * @param code HTTP error status code (4xx or 5xx)
 * @param errorPages Map of status codes to HTML file paths
 *
 * @note Backward compatible via overload (can call without map)
 * @note Always safe (fallback ensures response is always valid)
 * @note File paths relative to server root (from LocationConfig)
 */
void HttpResponse::setErrorResponse(int code, const std::map<int, std::string> &errorPages)
{
    _httpVersion = "HTTP/1.1";
    _statusCode = code;

    std::map<int, std::string>::const_iterator it = errorPages.find(code);
    if (it != errorPages.end())
    {
        std::string errorContent = readErrorFile(it->second);
        if (!errorContent.empty())
        {
            _httpVersion = "HTTP/1.1";
            _statusCode = code;
            _statusMessage = getStatusMessage(code);
            _body = errorContent;
            _headers["Content-Type"] = "text/html";
            _headers["Content-Length"] = sizeToString(_body.size());
            return;
        }
    }
    setErrorResponse(code);
}

// ==================== RESPONSE BUILDER ====================

/**
 * @brief Builds complete HTTP response string ready for socket transmission
 *
 * Constructs final HTTP response by combining status line, headers, and body.
 * Automatically adds required headers (Server, Date, Content-Length).
 *
 * HTTP response format:
 *   STATUS_LINE\r\n
 *   HEADER1: value1\r\n
 *   HEADER2: value2\r\n
 *   \r\n
 *   BODY
 *
 * Automatic headers added:
 * - Server: webserv/1.0 (identifies server software)
 * - Date: Current GMT timestamp (RFC 9110 required)
 * - Content-Length: Body size in bytes (only if body exists)
 *
 * Why headers added here (not constructor):
 * - Date header must be fresh (current time when response sent)
 * - Content-Length depends on body (may change after construction)
 *
 * Why Content-Length conditional:
 * - Only added if body exists AND not already set manually
 * - Prevents duplicate header
 * - Respects manual override if needed
 *
 * Usage example:
 *   HttpResponse response;
 *   response.setStatus(200, "OK");
 *   response.setBody("<html>...</html>");
 *   std::string http = response.buildResponse();
 *   send(socket, http.c_str(), http.size(), 0);
 *
 * @return Complete HTTP response as string with \r\n line endings
 *
 * @note Call this once before sending to socket
 * @note Modifies _headers (adds Server, Date, Content-Length)
 * @note Not const because it modifies _headers map
 * @note Uses ostringstream for efficient string concatenation
 */
std::string HttpResponse::buildResponse()
{
    setHeader("Server", "webserv/1.0");
    setHeader("Date", getHttpDate());

    std::ostringstream oss;
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    if (!_body.empty() && _headers.find("Content-Length") == _headers.end())
    {
        _headers["Content-Length"] = sizeToString(_body.size());
    }
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end();
         ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }
    oss << "\r\n";
    oss << _body;

    return oss.str();
}