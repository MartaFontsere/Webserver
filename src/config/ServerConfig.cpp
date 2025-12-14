#include "../../includes/config/ServerConfig.hpp"
/**
 * @file ServerConfig.cpp
 * @brief Server block configuration storage - Represents nginx server directive
 *
 * This module stores all configuration parameters for a single server block
 * within an HTTP context. Server blocks define virtual hosts and contain
 * location blocks for request routing.
 *
 * Configuration structure:
 *   server {
 *       listen 8080;
 *       host 127.0.0.1;
 *       server_name localhost example.com;
 *       root ./www;
 *       index index.html index.php;
 *       client_max_body_size 1048576;
 *       error_page 404 /404.html;
 *
 *       location / { ... }
 *       location /api { ... }
 *   }
 *
 * Managed data (8 attributes):
 * - Network binding (port and host address)
 * - Virtual host identification (server names)
 * - Default document serving (root, index files)
 * - Error handling (custom error pages)
 * - Request limits (max body size)
 * - Location blocks (vector of LocationConfig for routing)
 *
 * Hierarchical structure:
 *   ServerConfig (1 server)
 *       └── Contains multiple LocationConfig (N locations)
 *
 * Design decisions:
 * - Class instead of struct (encapsulation for future validation)
 * - STL containers only (no manual memory management)
 * - Getters return const& for containers (avoid expensive copies)
 * - Default port 0 (must be configured), default body size 1MB
 *
 * Integration points:
 * - Built by ConfigBuilder::buildServer()
 * - Used by Server class for socket binding (listen, host)
 * - Used by RequestRouter for virtual host matching (serverNames)
 * - Contains LocationConfig vector for request routing
 *
 * @note One ServerConfig per server block in configuration
 * @see ConfigBuilder::buildServer() for construction from parsed config
 */

/**
 * @brief Default constructor - Initializes server with safe defaults
 *
 * Creates a ServerConfig with empty containers (STL auto-initialization)
 * and sensible defaults for primitive types.
 *
 * Default values:
 * - _listen = 0 (unset, must be configured)
 * - _host = "" (empty, typically "127.0.0.1" or "0.0.0.0")
 * - _serverNames = [] (empty vector, should have at least one name)
 * - _root = "" (empty, should be set for static file serving)
 * - _index = [] (empty, common default would be ["index.html"])
 * - _errorPages = {} (empty map, will show default error pages)
 * - _clientMaxBodySize = 1048576 (1 MB default, prevents huge uploads)
 * - _locations = [] (empty, should have at least one location)
 *
 * Constructor initialization list only includes primitives because:
 * - STL containers initialize to empty automatically
 * - Primitives contain garbage without explicit initialization
 *
 * @note _listen = 0 is used as sentinel (invalid port, must be set)
 * @note Called by ConfigBuilder when creating new server configurations
 */
ServerConfig::ServerConfig() : _listen(0), _clientMaxBodySize(1048576)
{
}

/**
 * @brief Copy constructor - Deep copies all configuration from another server
 *
 * Creates a new ServerConfig by copying all 8 members from an existing
 * server configuration. All STL containers perform deep copies automatically,
 * including the vector of LocationConfig objects.
 *
 * Copy behavior:
 * - Strings: Deep copy
 * - Vectors: Deep copy with all elements (including LocationConfig objects)
 * - Map: Deep copy with all key-value pairs
 * - Primitives: Value copy
 *
 * Heavy operation warning:
 * - Copying vector<LocationConfig> can be expensive if many locations exist
 * - Each LocationConfig is also deep-copied (12 members each)
 *
 * Usage example:
 *   ServerConfig srv1;
 *   srv1.setListen(8080);
 *   ServerConfig srv2(srv1);  // Independent copy
 *   srv2.setListen(9090);     // Doesn't affect srv1
 *
 * @param other ServerConfig to copy from
 *
 * @note All 8 members copied in initialization list
 * @note Locations vector performs deep copy of all LocationConfig objects
 */
ServerConfig::ServerConfig(const ServerConfig &other)
    : _listen(other._listen), _host(other._host), _serverNames(other._serverNames),
      _root(other._root), _index(other._index), _errorPages(other._errorPages),
      _clientMaxBodySize(other._clientMaxBodySize), _locations(other._locations)
{
}

/**
 * @brief Assignment operator - Copies configuration from another server
 *
 * Overwrites current configuration with values from another ServerConfig.
 * All members are STL containers or primitives, so direct assignment is safe.
 *
 * Self-assignment protection:
 * - if (this != &other) prevents srv1 = srv1 bugs
 * - Required by OCF even though STL handles self-assignment safely
 *
 * Assignment behavior:
 * - All 8 members assigned individually
 * - STL containers handle deep copy (including vector<LocationConfig>)
 * - Primitives copied by value
 *
 * Usage example:
 *   ServerConfig srv1, srv2;
 *   srv1.setListen(8080);
 *   srv2 = srv1;              // Copy all configuration
 *   srv2.setListen(9090);     // Independent modification
 *
 * @param other ServerConfig to copy from
 * @return Reference to this object (*this) for chained assignments (a = b = c)
 *
 * @note Returns *this to enable assignment chaining
 * @note Self-assignment check required by OCF
 */
ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        _listen = other._listen;
        _host = other._host;
        _serverNames = other._serverNames;
        _root = other._root;
        _index = other._index;
        _errorPages = other._errorPages;
        _clientMaxBodySize = other._clientMaxBodySize;
        _locations = other._locations;
    }
    return *this;
}

/**
 * @brief Destructor - Automatic cleanup via RAII
 *
 * Destroys the ServerConfig object. No manual cleanup needed because:
 * - All members are STL containers or primitives
 * - STL destructors handle memory release automatically
 * - vector<LocationConfig> destroys all LocationConfig objects
 *
 * Destruction cascade:
 * 1. ~ServerConfig() called
 * 2. ~std::vector<LocationConfig>() destroys each LocationConfig
 * 3. Each LocationConfig destructor cleans its own members
 * 4. All memory released
 *
 * @note Empty body because STL handles all cleanup (RAII principle)
 * @note No manual delete/free needed
 */
ServerConfig::~ServerConfig()
{
}

// ==================== GETTERS ====================

/**
 * @brief Returns server listen port
 * @return Port number (e.g., 8080, 80, 443)
 * @note Returns 0 if not configured (invalid port, must be set)
 */
int ServerConfig::getListen() const
{
    return _listen;
}

/**
 * @brief Returns server bind host address
 * @return Reference to host IP address string
 * @note Common values: "127.0.0.1", "0.0.0.0", "::1"
 */
const std::string &ServerConfig::getHost() const
{
    return _host;
}

/**
 * @brief Returns list of server names (virtual hosts)
 * @return Reference to vector of server name strings
 * @note Example: ["localhost", "example.com", "www.example.com"]
 */
const std::vector<std::string> &ServerConfig::getServerNames() const
{
    return _serverNames;
}

/**
 * @brief returns server's default document root
 * @return Reference to root directory path
 * @note Used as fallback when location doesn't specify root
 */
const std::string &ServerConfig::getRoot() const
{
    return _root;
}

/**
 * @brief Returns default index files for the server
 * @return Reference to vector of index filenames
 * @note Example: ["index.html", "index.php"]
 */
const std::vector<std::string> &ServerConfig::getIndex() const
{
    return _index;
}

/**
 * @brief Returns custom error page mappings
 * @return Reference to map of error codes to HTML file paths
 * @note Example: {404: "/404.html", 500: "/500.html"}
 */
const std::map<int, std::string> &ServerConfig::getErrorPages() const
{
    return _errorPages;
}

/**
 * @brief Returns maximum allowed request body size
 * @return Maximum body size in bytes (default 1048576 = 1MB)
 * @note Used to prevent oversized requests/uploads
 */
size_t ServerConfig::getClientMaxBodySize() const
{
    return _clientMaxBodySize;
}

/**
 * @brief Returns all location blocks configured for this server
 * @return Reference to vector of LocationConfig objects
 * @note Each location handles requests matching its pattern
 */
const std::vector<LocationConfig> &ServerConfig::getLocations() const
{
    return _locations;
}

// ==================== SETTERS ====================

/**
 * @brief Sets server listen port
 * @param listen Port number to bind (1-65535)
 * @note Common ports: 80 (HTTP), 443 (HTTPS), 8080 (development)
 */
void ServerConfig::setListen(int listen)
{
    _listen = listen;
}

/**
 * @brief Sets server bind host address
 * @param host IP address to bind to
 * @note Examples: "127.0.0.1" (localhost), "0.0.0.0" (all interfaces)
 */
void ServerConfig::setHost(const std::string &host)
{
    _host = host;
}

/**
 * @brief Sets server names for virtual host matching
 * @param serverNames Vector of domain names this server responds to
 * @note Example: ["localhost", "example.com", "*.example.com"]
 */
void ServerConfig::setServerNames(const std::vector<std::string> &serverNames)
{
    _serverNames = serverNames;
}

/**
 * @brief Sets server's default document root
 * @param root Directory path for serving files
 * @note Locations can override with their own root
 */
void ServerConfig::setRoot(const std::string &root)
{
    _root = root;
}

/**
 * @brief Sets default index files for the server
 * @param index Vector of filenames to try when directory requested
 * @note Example: ["index.html", "index.php", "default.html"]
 */
void ServerConfig::setIndex(const std::vector<std::string> &index)
{
    _index = index;
}

/**
 * @brief Sets custom error page mappings for the server
 * @param errorPages Map of HTTP status codes to error page paths
 * @note Example: {404: "/404.html", 500: "/500.html"}
 */
void ServerConfig::setErrorPages(const std::map<int, std::string> &errorPages)
{
    _errorPages = errorPages;
}

/**
 * @brief Sets maximum request body size in bytes
 * @param clientMaxBodySize Maximum allowed body size
 * @note Common values: 1048576 (1MB), 10485760 (10MB), 104857600 (100MB)
 */
void ServerConfig::setClientMaxBodySize(size_t clientMaxBodySize)
{
    _clientMaxBodySize = clientMaxBodySize;
}

/**
 * @brief Sets all location blocks for this server
 * @param locations Vector of LocationConfig objects
 * @note Each location defines routing rules for URI patterns
 */
void ServerConfig::setLocations(const std::vector<LocationConfig> &locations)
{
    _locations = locations;
}