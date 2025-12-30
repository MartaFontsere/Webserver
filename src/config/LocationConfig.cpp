#include "../../includes/config/LocationConfig.hpp"

/**
 * @file LocationConfig.cpp
 * @brief Location block configuration storage - Represents nginx location
 * directive
 *
 * This module stores all configuration parameters for a single location
 * block within a server configuration. Location blocks define how the
 * server handles requests matching specific URI patterns.
 *
 * Configuration structure:
 *   location [pattern] {
 *       root ./www;
 *       index index.html;
 *       allow_methods GET POST;
 *       autoindex on;
 *       cgi_path /usr/bin/php-cgi;
 *       cgi_ext .php;
 *       error_page 404 /404.html;
 *       return 301 /new-location;
 *       upload_path ./uploads;
 *       client_max_body_size 1048576;
 *   }
 *
 * Managed data (12 attributes):
 * - Pattern matching (URI pattern for this location)
 * - Static file serving (root, index, autoindex)
 * - HTTP methods (GET, POST, DELETE allowed)
 * - CGI execution (interpreter paths and extensions)
 * - Error handling (custom error pages per status code)
 * - Redirects (HTTP redirects with status code)
 * - File uploads (upload directory and size limits)
 *
 * Design decisions:
 * - Class instead of struct (encapsulation, validation-ready)
 * - STL containers only (no manual memory management)
 * - Getters return const& for containers (avoid copies)
 * - Default values match common web server defaults
 *
 * Integration points:
 * - Built by ConfigBuilder::buildLocation()
 * - Used by CGIHandler::handle() (cgiPaths, cgiExts)
 * - Used by StaticFileHandler (root, index, autoindex)
 * - Used by UploadHandler (uploadPath, bodySize)
 * - Used by RequestRouter (methods, returnCode, errorPages)
 *
 * @note Replaces hardcoded LocationConfig mock from CGI module
 * @see ConfigBuilder::buildLocation() for construction from parsed config
 */

/**
 * @brief Default constructor - Initializes location with safe defaults
 *
 * Creates a LocationConfig with empty containers (STL auto-initialization)
 * and sensible defaults for primitive types.
 *
 * Default values:
 * - _root = "" (empty, must be set)
 * - _index = [] (empty vector)
 * - _methods = [] (empty, should be set to restrict methods)
 * - _cgiPaths = [] (empty, CGI disabled by default)
 * - _cgiExts = [] (empty)
 * - _errorPages = {} (empty map, server defaults will apply)
 * - _returnCode = 0 (no redirect configured)
 * - _returnUrl = "" (no redirect)
 * - _bodySize = -1 (no limit)
 * - _pattern = "" (must be set from location block name)
 * - _uploadPath = "" (uploads disabled by default)
 * - _autoindex = false (directory listing disabled by default)
 *
 * Constructor initialization list only includes primitives because:
 * - STL containers (string, vector, map) initialize to empty automatically
 * - Primitives (int, size_t, bool) contain garbage without initialization
 *
 * @note Called by ConfigBuilder when creating new location configurations
 * @note All STL members auto-initialize to empty (not in initialization
 * list)
 */

/*
Cambio de body size de 1048576 (1MB) a -1 (no limit). El valor -1 es un
valor centinela, es decir un valor especial que no es válido como valor real
(no tiene sentido como tamaño del body), y que significa "esto no ha sido
configurado".

De este modo, podemos distinguir el origen del body size, es decir, si el
usuario lo definió explicitamente o si simplemente es el valor por defecto
del constructor (esta location no define el client max body size). esto
desbloquea la herencia real.

Si usamos 1048576 como default, no podemos implementar correctamente la
herencia, overrides y la lógica de prioridad.
*/

LocationConfig::LocationConfig()
    : _returnCode(0), _maxBodySize(static_cast<size_t>(-1)), _alias(""),
      _autoindex(false) {}

/**
 * @brief Copy constructor - Deep copies all configuration from another
 * location
 *
 * Creates a new LocationConfig by copying all 12 members from an existing
 * location. All STL containers perform deep copies automatically (vector
 * and map copy constructors handle internal memory).
 *
 * Copy behavior:
 * - Strings: Deep copy (std::string copy constructor)
 * - Vectors: Deep copy with all elements (std::vector copy constructor)
 * - Map: Deep copy with all key-value pairs (std::map copy constructor)
 * - Primitives: Value copy (int, size_t, bool)
 *
 * Usage example:
 *   LocationConfig loc1;
 *   loc1.setRoot("./www");
 *   LocationConfig loc2(loc1);  // loc2 is independent copy
 *   loc2.setRoot("./other");    // Doesn't affect loc1
 *
 * @param other LocationConfig to copy from
 *
 * @note All 12 members copied in initialization list (efficient
 * construction)
 * @note No manual memory management needed (STL handles everything)
 */
LocationConfig::LocationConfig(const LocationConfig &other)
    : _root(other._root), _index(other._index), _methods(other._methods),
      _cgiPaths(other._cgiPaths), _cgiExts(other._cgiExts),
      _errorPages(other._errorPages), _returnCode(other._returnCode),
      _returnUrl(other._returnUrl), _maxBodySize(other._maxBodySize),
      _pattern(other._pattern), _uploadPath(other._uploadPath),
      _alias(other._alias), _autoindex(other._autoindex) {}

/**
 * @brief Assignment operator - Copies configuration from another location
 *
 * Overwrites current configuration with values from another LocationConfig.
 * Implements the copy-and-swap idiom without swap (direct assignment is
 * safe because all members are STL containers or primitives).
 *
 * Self-assignment protection:
 * - if (this != &other) prevents loc1 = loc1 bugs
 * - Critical for safety even though STL handles self-assignment
 * - OCF requirement (Orthodox Canonical Form)
 *
 * Assignment behavior (same as copy constructor):
 * - All 12 members assigned individually
 * - STL containers handle deep copy automatically
 * - Primitives copied by value
 *
 * Usage example:
 *   LocationConfig loc1, loc2;
 *   loc1.setRoot("./www");
 *   loc2 = loc1;              // Copy all configuration
 *   loc2.setRoot("./other");  // Independent modification
 *
 * @param other LocationConfig to copy from
 * @return Reference to this object (*this) for chained assignments (a = b =
 * c)
 *
 * @note Returns *this to enable chaining: loc1 = loc2 = loc3
 * @note Self-assignment check required by OCF even though safe without it
 */
LocationConfig &LocationConfig::operator=(const LocationConfig &other) {
  if (this != &other) {
    _root = other._root;
    _index = other._index;
    _methods = other._methods;
    _cgiPaths = other._cgiPaths;
    _cgiExts = other._cgiExts;
    _errorPages = other._errorPages;
    _returnCode = other._returnCode;
    _returnUrl = other._returnUrl;
    _maxBodySize = other._maxBodySize;
    _pattern = other._pattern;
    _uploadPath = other._uploadPath;
    _alias = other._alias;
    _autoindex = other._autoindex;
  }
  return *this;
}

/**
 * @brief Destructor - Automatic cleanup via RAII
 *
 * Destroys the LocationConfig object. No manual cleanup needed because:
 * - std::string members destroy themselves (RAII)
 * - std::vector members destroy themselves (RAII)
 * - std::map members destroy themselves (RAII)
 * - Primitives (int, size_t, bool) require no cleanup
 *
 * Destruction order (automatic):
 * 1. ~LocationConfig() called
 * 2. After this function, member destructors called automatically:
 *    - ~std::string() for _root, _returnUrl, _pattern, _uploadPath
 *    - ~std::vector() for _index, _methods, _cgiPaths, _cgiExts
 *    - ~std::map() for _errorPages
 * 3. All memory released by STL destructors
 *
 * @note Empty body because STL handles all cleanup (RAII principle)
 * @note No manual delete/free needed (no raw pointers or resources)
 */
LocationConfig::~LocationConfig() {}

// ==================== GETTERS ====================

/**
 * @brief Returns document root path for this location
 * @return Reference to root directory path
 * @note Returns const reference (no copy, safe read-only access)
 */
const std::string &LocationConfig::getRoot() const { return _root; }

/**
 * @brief Returns list of index files to serve for directories
 * @return Reference to vector of index filenames
 * @note Common values: ["index.html", "index.php"]
 */
const std::vector<std::string> &LocationConfig::getIndex() const {
  return _index;
}

/**
 * @brief Returns list of allowed HTTP methods for this location
 * @return Reference to vector of HTTP method names
 * @note Common values: ["GET", "POST", "DELETE"]
 */
const std::vector<std::string> &LocationConfig::getMethods() const {
  return _methods;
}

/**
 * @brief Returns list of CGI interpreter paths
 * @return Reference to vector of CGI executable paths
 * @note Example: ["/usr/bin/php-cgi", "/usr/bin/python3"]
 */
const std::vector<std::string> &LocationConfig::getCgiPaths() const {
  return _cgiPaths;
}

/**
 * @brief Returns list of CGI file extensions
 * @return Reference to vector of file extensions
 * @note Example: [".php", ".py", ".sh"]
 */
const std::vector<std::string> &LocationConfig::getCgiExts() const {
  return _cgiExts;
}

/**
 * @brief Returns custom error page mappings (code → file path)
 * @return Reference to map of error codes to HTML file paths
 * @note Example: {404: "/404.html", 500: "/500.html"}
 */
const std::map<int, std::string> &LocationConfig::getErrorPages() const {
  return _errorPages;
}

/**
 * @brief Returns HTTP redirect status code (0 if no redirect)
 * @return Redirect code (301, 302, etc.) or 0 for no redirect
 * @note 0 indicates no redirect configured for this location
 */
int LocationConfig::getReturnCode() const { return _returnCode; }

/**
 * @brief Returns redirect destination URL
 * @return Reference to redirect URL string
 * @note Empty string if no redirect configured
 */
const std::string &LocationConfig::getReturnUrl() const { return _returnUrl; }

/**
 * @brief Returns maximum allowed request body size in bytes
 * @return Maximum body size (default 1048576 = 1 MB)
 * @note Used to prevent oversized uploads/POST requests
 */
size_t LocationConfig::getMaxBodySize() const { return _maxBodySize; }

/**
 * @brief Returns URI pattern for this location
 * @return Reference to location pattern string
 * @note Example: "/", "/admin", "~ \.php$"
 */
const std::string &LocationConfig::getPattern() const { return _pattern; }

/**
 * @brief Returns directory path for file uploads
 * @return Reference to upload directory path
 * @note Empty string if uploads not configured
 */
const std::string &LocationConfig::getUploadPath() const { return _uploadPath; }

/**
 * @brief Returns directory listing (autoindex) setting
 * @return true if autoindex enabled, false otherwise
 * @note When true, generates HTML directory listings
 */
bool LocationConfig::getAutoindex() const { return _autoindex; }

/**
 * @brief Checks if an HTTP method is allowed in this location
 * @param method Method name to check (GET, POST, DELETE)
 * @return true if allowed or if no methods are restricted, false otherwise
 */
bool LocationConfig::isMethodAllowed(const std::string &method) const {
  if (_methods.empty())
    return true;
  for (size_t i = 0; i < _methods.size(); ++i) {
    if (_methods[i] == method)
      return true;
  }
  return false;
}

/**
 * @brief Checks if file uploads are enabled for this location
 * @return true if an upload path is configured, false otherwise
 */
bool LocationConfig::isUploadEnabled() const { return !_uploadPath.empty(); }

/**
 * @brief Checks if an alias is configured for this location
 * @return true if alias is set, false otherwise
 */
bool LocationConfig::hasAlias() const { return !_alias.empty(); }

/**
 * @brief Returns the alias path
 * @return Reference to alias string
 */
const std::string &LocationConfig::getAlias() const { return _alias; }

// ==================== SETTERS ====================

/**
 * @brief Sets document root path for this location
 * @param root Directory path to serve files from
 * @note Example: "./www", "/var/www/html"
 */
void LocationConfig::setRoot(const std::string &root) { _root = root; }

/**
 * @brief Sets list of index files to serve for directories
 * @param index Vector of index filenames to try in order
 * @note Example: ["index.html", "index.php", "default.html"]
 */
void LocationConfig::setIndex(const std::vector<std::string> &index) {
  _index = index;
}

/**
 * @brief Sets allowed HTTP methods for this location
 * @param methods Vector of HTTP method names to allow
 * @note Example: ["GET", "POST", "DELETE"]
 */
void LocationConfig::setMethods(const std::vector<std::string> &methods) {
  _methods = methods;
}

/**
 * @brief Sets CGI interpreter executable paths
 * @param cgiPaths Vector of CGI interpreter paths
 * @note Example: ["/usr/bin/php-cgi", "/usr/bin/python3"]
 */
void LocationConfig::setCgiPaths(const std::vector<std::string> &cgiPaths) {
  _cgiPaths = cgiPaths;
}

/**
 * @brief Sets CGI file extensions to handle
 * @param cgiExts Vector of file extensions for CGI scripts
 * @note Example: [".php", ".py", ".sh"]
 */
void LocationConfig::setCgiExts(const std::vector<std::string> &cgiExts) {
  _cgiExts = cgiExts;
}

/**
 * @brief Sets custom error page mappings
 * @param errorPages Map of HTTP error codes to HTML file paths
 * @note Example: {404: "/404.html", 500: "/500.html", 403: "/403.html"}
 */
void LocationConfig::setErrorPages(
    const std::map<int, std::string> &errorPages) {
  _errorPages = errorPages;
}

/**
 * @brief Sets HTTP redirect status code
 * @param returnCode HTTP status code for redirect (301, 302, etc.)
 * @note Set to 0 to disable redirect
 */
void LocationConfig::setReturnCode(int returnCode) { _returnCode = returnCode; }

/**
 * @brief Sets redirect destination URL
 * @param returnUrl URL to redirect requests to
 * @note Used together with returnCode (e.g., 301 permanent redirect)
 */
void LocationConfig::setReturnUrl(const std::string &returnUrl) {
  _returnUrl = returnUrl;
}

/**
 * @brief Sets maximum request body size in bytes
 * @param bodySize Maximum allowed body size
 * @note Common values: 1048576 (1MB), 10485760 (10MB)
 */
void LocationConfig::setMaxBodySize(size_t maxBodySize) {
  _maxBodySize = maxBodySize;
}

/**
 * @brief Sets URI pattern for location matching
 * @param pattern URI pattern string
 * @note Example: "/", "/admin", "~ \.php$" (regex), "^~ /images/"
 */
void LocationConfig::setPattern(const std::string &pattern) {
  _pattern = pattern;
}

/**
 * @brief Sets directory for uploaded files
 * @param uploadPath Directory path to store uploads
 * @note Example: "./uploads", "/tmp/uploads"
 */
void LocationConfig::setUploadPath(const std::string &uploadPath) {
  _uploadPath = uploadPath;
}

/**
 * @brief Sets the alias path for this location
 * @param alias Path to use as alias
 */
void LocationConfig::setAlias(const std::string &alias) { _alias = alias; }

/**
 * @brief Sets directory listing (autoindex) mode
 * @param autoindex true to enable directory listings, false to disable
 * @note When true, generates HTML listings when no index file found
 */
void LocationConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }