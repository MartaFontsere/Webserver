#include "../../includes/config/ConfigBuilder.hpp"
/**
 * @file ConfigBuilder.cpp
 * @brief Configuration builder - Converts BlockParser tree to usable
 * ServerConfig/LocationConfig
 *
 * This module orchestrates the conversion from parsed configuration
 * (BlockParser tree) to typed, structured configuration objects
 * (ServerConfig and LocationConfig). It bridges the gap between syntax
 * parsing and runtime configuration.
 *
 * Conversion pipeline:
 *   BlockParser (generic tree of blocks/directives)
 *        ↓
 *   ConfigBuilder (extraction and conversion)
 *        ↓
 *   ServerConfig + LocationConfig (typed structures ready for use)
 *
 * Architecture (3 layers):
 * 1. Generic helpers (getDirectiveValue, getDirectiveValues,
 * getDirectiveValueAsInt)
 * 2. Specialized parsers (parseAutoindex, parseReturn, parseErrorPages)
 * 3. Builders (buildLocation, buildServer, buildFromBlockParser)
 *
 * Responsibilities:
 * - Extract directive values from BlockParser
 * - Convert string values to appropriate types (int, bool, vectors, maps)
 * - Handle special cases (error_page, return, autoindex)
 * - Validate data completeness (size checks for multi-value directives)
 * - Build hierarchical configuration (servers contain locations)
 *
 * Error handling strategy:
 * - Missing directives: Use defaults (empty strings, empty vectors, 0)
 * - Malformed directives: Skip silently (validation done by
 * SemanticValidator)
 * - Invalid conversions: Return 0 or empty (graceful degradation)
 *
 * @note Stateless class - no member variables, all state is method-local
 * @see ServerConfig for server-level configuration structure
 * @see LocationConfig for location-level configuration structure
 */

/**
 * @brief Default constructor - Initializes stateless ConfigBuilder
 *
 * Creates a ConfigBuilder object. The class has no member variables,
 * so the constructor is empty. All conversion logic is contained within
 * method-local variables.
 *
 * Design rationale:
 * - Stateless design allows reuse across multiple configurations
 * - No shared state = thread-safe by design
 * - Can add state later (error counting, warnings) if needed
 *
 * @note Empty constructor because class is stateless
 */
ConfigBuilder::ConfigBuilder() {}

/**
 * @brief Destructor - No cleanup needed for stateless class
 *
 * Destroys the ConfigBuilder object. No cleanup required because:
 * - No member variables to destroy
 * - All resources are method-local (managed by method scopes)
 *
 * @note Empty destructor because class owns no resources
 */
ConfigBuilder::~ConfigBuilder() {}

/**
 * @brief Extracts first value of a directive by name
 *
 * Searches for a specific directive in a block and returns its first value.
 * Used for single-value directives like "root", "host", "upload_path".
 *
 * Search algorithm:
 * 1. Get all directives from block
 * 2. Iterate through directive vector
 * 3. Find directive with matching name
 * 4. If found and has values: return first value
 * 5. If not found or empty: return ""
 *
 * Common use cases:
 *   getDirectiveValue(block, "root") → "./www"
 *   getDirectiveValue(block, "host") → "127.0.0.1"
 *   getDirectiveValue(block, "upload_path") → "./uploads"
 *
 * @param block BlockParser containing directives to search
 * @param directiveName Name of directive to find (e.g., "root", "host")
 * @return First value of directive, or empty string if not found
 *
 * @note Returns empty string for missing/empty directives (safe default)
 * @note Case-sensitive directive name matching
 */
std::string ConfigBuilder::getDirectiveValue(const BlockParser &block,
                                             const std::string &directiveName) {
  std::vector<DirectiveToken> directives = block.getDirectives();

  for (size_t i = 0; i < directives.size(); ++i) {
    if (directives[i].name == directiveName) {
      if (!directives[i].values.empty()) {
        return directives[i].values[0];
      }
    }
  }
  return "";
}

/**
 * @brief Extracts all values of a directive by name
 *
 * Searches for a specific directive and returns all its values as a vector.
 * Used for multi-value directives like "index", "allow_methods", "cgi_ext".
 *
 * Examples:
 *   Directive: index index.html index.php index.htm;
 *   Result: ["index.html", "index.php", "index.htm"]
 *
 *   Directive: allow_methods GET POST DELETE;
 *   Result: ["GET", "POST", "DELETE"]
 *
 *   Directive: server_name localhost example.com;
 *   Result: ["localhost", "example.com"]
 *
 * @param block BlockParser containing directives to search
 * @param directiveName Name of directive to find
 * @return Vector of all directive values, or empty vector if not found
 *
 * @note Returns empty vector for missing directives (safe default)
 * @note Returns all values (different from getDirectiveValue which returns only
 * first)
 */
std::vector<std::string>
ConfigBuilder::getDirectiveValues(const BlockParser &block,
                                  const std::string &directiveName) {
  std::vector<DirectiveToken> directives = block.getDirectives();

  for (size_t i = 0; i < directives.size(); ++i) {
    if (directives[i].name == directiveName) {
      if (!directives[i].values.empty()) {
        return directives[i].values;
      }
    }
  }
  return std::vector<std::string>();
}

/**
 * @brief Extracts directive value and converts to integer
 *
 * Convenience method that combines getDirectiveValue() with string-to-int
 * conversion. Used for numeric directives like "listen",
 * "client_max_body_size".
 *
 * Conversion process:
 * 1. Extract first value as string
 * 2. If found: convert using stringToInt()
 * 3. If not found: return 0 (safe default)
 *
 * Common use cases:
 *   getDirectiveValueAsInt(block, "listen") → 8080
 *   getDirectiveValueAsInt(block, "client_max_body_size") → 1048576
 *
 * Error cases:
 *   Directive missing → returns 0
 *   Directive empty → returns 0
 *   Invalid number → returns 0 (stringToInt fails silently)
 *
 * @param block BlockParser containing directives to search
 * @param directiveName Name of numeric directive to find
 * @return Parsed integer value, or 0 if not found/invalid
 *
 * @note Uses stringToInt() from UtilsConfig for conversion
 * @note Returns 0 as default (may be valid value - caller should validate)
 */
int ConfigBuilder::getDirectiveValueAsInt(const BlockParser &block,
                                          const std::string &directiveName) {
  std::string directive = getDirectiveValue(block, directiveName);
  if (!directive.empty()) {
    int value = stringToInt(directive);
    return value;
  }
  return 0;
}

/**
 * @brief Parses autoindex directive and sets boolean value in location
 *
 * Extracts "autoindex" directive value and converts to boolean:
 * - "on" → true (directory listing enabled)
 * - "off" or missing → false (directory listing disabled)
 *
 * Directive format:
 *   autoindex on;   → true
 *   autoindex off;  → false
 *
 * @param locationBlock BlockParser of location to extract from
 * @param location LocationConfig to modify (passed by reference)
 *
 * @note Modifies location in-place via setAutoindex()
 * @note Defaults to false for any value other than "on"
 */
void ConfigBuilder::parseAutoindex(const BlockParser &locationBlock,
                                   LocationConfig &location) {
  std::string autoindexValue = getDirectiveValue(locationBlock, "autoindex");
  if (autoindexValue == "on")
    location.setAutoindex(true);
  else
    location.setAutoindex(false);
}

/**
 * @brief Parses return directive with validation for code and URL
 *
 * Extracts "return" directive and handles 3 cases:
 * - 2+ values: Set both code and URL (normal case)
 * - 1 value: Set only code (URL-less redirect)
 * - 0 values: Set code to 0 (no redirect)
 *
 * Directive format:
 *   return 301 https://new-site.com;  → code=301, url="https://..."
 *   return 444;                        → code=444, url="" (nginx special)
 *   (missing)                          → code=0, url=""
 *
 * Validation:
 * - Checks vector size before accessing elements
 * - Prevents crashes on malformed directives
 * - Graceful degradation (sets code=0 if completely missing)
 *
 * @param locationBlock BlockParser of location to extract from
 * @param location LocationConfig to modify (passed by reference)
 *
 * @note Modifies location in-place via setReturnCode() and setReturnUrl()
 * @note returnCode=0 is used as sentinel (no redirect configured)
 */
void ConfigBuilder::parseReturn(const BlockParser &locationBlock,
                                LocationConfig &location) {
  std::vector<std::string> returnValues =
      getDirectiveValues(locationBlock, "return");
  if (returnValues.size() >= 2) {
    int valueInt = 0;
    valueInt = stringToInt(returnValues[0]);
    location.setReturnCode(valueInt);
    location.setReturnUrl(returnValues[1]);
  } else if (returnValues.size() == 1) {
    int valueInt = 0;
    valueInt = stringToInt(returnValues[0]);
    location.setReturnCode(valueInt);
  } else
    location.setReturnCode(0);
}

/**
 * @brief Parses all error_page directives and builds error code → file map
 *
 * Searches for ALL occurrences of "error_page" directive in a location block
 * and constructs a map from error codes to custom error page paths.
 *
 * Why manual parsing (not using helper):
 * - error_page can appear MULTIPLE times in same block
 * - getDirectiveValues() only returns first occurrence
 * - Need ALL occurrences to build complete map
 *
 * Directive format:
 *   error_page 404 /404.html;
 *   error_page 500 /500.html;
 *   error_page 403 /403.html;
 *
 * Result map:
 *   {404: "/404.html", 500: "/500.html", 403: "/403.html"}
 *
 * Parsing algorithm:
 * 1. Get ALL directives from block
 * 2. Create empty map
 * 3. For each directive:
 *    - If name is "error_page" AND has 2+ values:
 *      * Convert first value to int (error code)
 *      * Use second value as file path
 *      * Insert into map
 * 4. Set complete map in location
 *
 * Validation:
 * - Checks values.size() >= 2 (prevents crash on malformed directives)
 * - Skips malformed entries silently (e.g., "error_page 404;")
 *
 * @param locationBlock BlockParser of location to extract from
 * @param location LocationConfig to modify (passed by reference)
 *
 * @note Handles multiple error_page directives (accumulates all in map)
 * @note Skips malformed directives without crashing
 */
void ConfigBuilder::locationParseErrorPages(const BlockParser &locationBlock,
                                            LocationConfig &location) {
  std::vector<DirectiveToken> directives = locationBlock.getDirectives();
  std::map<int, std::string> errorMap;
  for (size_t i = 0; i < directives.size(); ++i) {
    if (directives[i].name == "error_page") {
      if (directives[i].values.size() >= 2) {
        int code = stringToInt(directives[i].values[0]);
        std::string path = directives[i].values[1];
        errorMap[code] = path;
      }
    }
  }
  location.setErrorPages(errorMap);
}

/**
 * @brief Builds complete LocationConfig from parsed location block
 *
 * Main entry point for converting a location BlockParser to LocationConfig.
 * Extracts all directives and delegates special cases to helper methods.
 *
 * Extraction strategy:
 * - Pattern: From block name (location / → pattern="/")
 * - Simple directives: Direct extraction with helpers
 * - Complex directives: Delegated to specialized parsers
 *
 * Directives processed (12 total):
 * 1. pattern (from block name)
 * 2. root (single value)
 * 3. index (multiple values)
 * 4. client_max_body_size (int value)
 * 5. cgi_ext (multiple values)
 * 6. cgi_path (multiple values)
 * 7. allow_methods (multiple values)
 * 8. upload_path (single value)
 * 9. autoindex (special: string → bool)
 * 10. return (special: code + URL with validation)
 * 11. error_page (special: multiple directives → map)
 *
 * Method modularity:
 * - Simple directives: Inline setters with helpers (~8 lines)
 * - Complex directives: Delegated to parse*() methods (clean separation)
 *
 * @param locationBlock BlockParser representing location { ... } block
 * @return Complete LocationConfig ready for use
 *
 * @note Creates new LocationConfig (not modifying existing)
 * @note All directives optional (uses defaults if missing)
 * @see parseAutoindex(), parseReturn(), locationParseErrorPages() for complex
 * cases
 */
LocationConfig ConfigBuilder::buildLocation(const BlockParser &locationBlock) {
  LocationConfig location;

  std::string name = locationBlock.getName();
  if (name.find("location ") == 0) {
    name = name.substr(9);
  }
  location.setPattern(name);
  location.setRoot(getDirectiveValue(locationBlock, "root"));
  location.setIndex(getDirectiveValues(locationBlock, "index"));
  std::string bodySizeStr =
      getDirectiveValue(locationBlock, "client_max_body_size");
  if (!bodySizeStr.empty()) {
    location.setMaxBodySize(static_cast<size_t>(stringToInt(bodySizeStr)));
  }
  location.setCgiExts(getDirectiveValues(locationBlock, "cgi_ext"));
  location.setCgiPaths(getDirectiveValues(locationBlock, "cgi_path"));
  location.setMethods(getDirectiveValues(locationBlock, "allow_methods"));
  location.setUploadPath(getDirectiveValue(locationBlock, "upload_path"));
  location.setAlias(getDirectiveValue(locationBlock, "alias"));

  parseAutoindex(locationBlock, location);
  parseReturn(locationBlock, location);
  locationParseErrorPages(locationBlock, location);

  return location;
}

/**
 * @brief Parses all error_page directives for server block (same as location)
 *
 * Identical to locationParseErrorPages() but operates on ServerConfig.
 * Servers can have error pages that apply to all locations as defaults.
 *
 * @param serverBlock BlockParser of server to extract from
 * @param server ServerConfig to modify (passed by reference)
 *
 * @note Duplicate of locationParseErrorPages (could be refactored to generic
 * helper)
 * @see locationParseErrorPages() for detailed documentation
 */
void ConfigBuilder::serverParseErrorPages(const BlockParser &serverBlock,
                                          ServerConfig &server) {
  std::vector<DirectiveToken> directives = serverBlock.getDirectives();
  std::map<int, std::string> errorMap;
  for (size_t i = 0; i < directives.size(); ++i) {
    if (directives[i].name == "error_page") {
      if (directives[i].values.size() >= 2) {
        int code = stringToInt(directives[i].values[0]);
        std::string path = directives[i].values[1];
        errorMap[code] = path;
      }
    }
  }
  server.setErrorPages(errorMap);
}

/**
 * @brief Parses all location blocks within a server and builds LocationConfig
 * vector
 *
 * Extracts nested location blocks from server, converts each to LocationConfig,
 * and stores them in the server's location vector. This enables request
 * routing.
 *
 * Process:
 * 1. Get nested blocks from server (these are location blocks)
 * 2. Create empty vector for storing LocationConfig objects
 * 3. For each nested block:
 *    - Convert to LocationConfig using buildLocation()
 *    - Add to locations vector
 * 4. Set complete vector in ServerConfig
 *
 * Nested structure:
 *   server {
 *       location / { ... }      ← nestedBlocks[0]
 *       location /api { ... }   ← nestedBlocks[1]
 *       location /admin { ... } ← nestedBlocks[2]
 *   }
 *
 * Result: server._locations = [LocationConfig1, LocationConfig2,
 * LocationConfig3]
 *
 * @param serverBlock BlockParser of server containing location blocks
 * @param server ServerConfig to modify (passed by reference)
 *
 * @note Reuses buildLocation() for each location (modular design)
 * @note Server can have 0 locations (empty vector is valid)
 */

/*
Mejora:
 Se ha modificado la función para que si location no tiene definidos el root,
index, client_max_body_size, o error_pages, los herede automáticamente del
bloque server
*/

void ConfigBuilder::serverParseLocation(const BlockParser &serverBlock,
                                        ServerConfig &server) {
  std::vector<BlockParser> nestedBlocks = serverBlock.getNestedBlocks();
  std::vector<LocationConfig> locations;

  for (size_t i = 0; i < nestedBlocks.size(); i++) {
    LocationConfig loc = buildLocation(nestedBlocks[i]);
    if (loc.getRoot().empty()) {
      loc.setRoot(server.getRoot());
    }
    if (loc.getIndex().empty()) {
      loc.setIndex(server.getIndex());
    }
    if (loc.getMaxBodySize() == static_cast<size_t>(-1)) {
      loc.setMaxBodySize(server.getClientMaxBodySize());
    }

    // Merge error pages: location error pages override server ones
    std::map<int, std::string> mergedErrors = server.getErrorPages();
    const std::map<int, std::string> &locErrors = loc.getErrorPages();
    for (std::map<int, std::string>::const_iterator it = locErrors.begin();
         it != locErrors.end(); ++it) {
      mergedErrors[it->first] = it->second;
    }
    loc.setErrorPages(mergedErrors);

    locations.push_back(loc);
  }

  server.setLocations(locations);
}

/**
 * @brief Builds complete ServerConfig from parsed server block
 *
 * Main entry point for converting a server BlockParser to ServerConfig.
 * Extracts all server-level directives and processes nested location blocks.
 *
 * Extraction strategy:
 * - Server directives: Direct extraction with helpers (listen, host, etc.)
 * - Error pages: Delegated to serverParseErrorPages()
 * - Locations: Delegated to serverParseLocation()
 *
 * Directives processed (7 server-level + N locations):
 * 1. listen (int - port number)
 * 2. host (string - bind address)
 * 3. server_name (multiple - virtual host names)
 * 4. root (string - document root)
 * 5. index (multiple - default index files)
 * 6. client_max_body_size (int - max request body)
 * 7. error_page (special - multiple directives → map)
 * 8. location blocks (special - nested blocks → vector<LocationConfig>)
 *
 * @param serverBlock BlockParser representing server { ... } block
 * @return Complete ServerConfig with all locations
 *
 * @note Creates new ServerConfig (not modifying existing)
 * @note All directives optional (uses defaults if missing)
 * @see serverParseErrorPages() for error_page handling
 * @see serverParseLocation() for location block processing
 */
ServerConfig ConfigBuilder::buildServer(const BlockParser &serverBlock) {
  ServerConfig server;

  server.setListen(getDirectiveValueAsInt(serverBlock, "listen"));
  server.setHost(getDirectiveValue(serverBlock, "host"));
  server.setServerNames(getDirectiveValues(serverBlock, "server_name"));
  server.setRoot(getDirectiveValue(serverBlock, "root"));
  server.setIndex(getDirectiveValues(serverBlock, "index"));
  server.setClientMaxBodySize(
      getDirectiveValueAsInt(serverBlock, "client_max_body_size"));

  serverParseErrorPages(serverBlock, server);
  serverParseLocation(serverBlock, server);

  return server;
}

/**
 * @brief Main orchestrator - Converts complete config file to vector of servers
 *
 * Top-level conversion method that processes the entire configuration tree.
 * Handles nginx-style nested structure: root → http → server → location.
 *
 * Configuration hierarchy:
 *   root (file)
 *   ├── events { }
 *   └── http {               ← Target context
 *       ├── server { ... }   ← Convert these
 *       └── server { ... }   ← to ServerConfig
 *   }
 *
 * Algorithm:
 * 1. Get root-level blocks (events, http, stream, mail...)
 * 2. Filter for "http" blocks
 * 3. For each http block:
 *    - Get nested server blocks
 *    - Convert each server to ServerConfig
 *    - Accumulate in result vector
 * 4. Return complete vector of servers
 *
 * Multi-http support:
 * - Handles multiple http blocks (accumulates all servers)
 * - Handles 0 http blocks (returns empty vector)
 *
 * Examples:
 *   1 http block with 2 servers → returns vector of size 2
 *   2 http blocks with 1 server each → returns vector of size 2
 *   No http blocks → returns empty vector
 *
 * @param root BlockParser representing entire configuration file
 * @return Vector of all ServerConfig objects found in http contexts
 *
 * @note Filters for "http" blocks (ignores events, stream, mail)
 * @note Can return empty vector (valid - no servers configured)
 * @note This is the ONLY public method needed to convert full config
 * @see buildServer() for individual server conversion
 */
std::vector<ServerConfig>
ConfigBuilder::buildFromBlockParser(const BlockParser &root) {
  std::vector<BlockParser> rootBlocks = root.getNestedBlocks();
  std::vector<ServerConfig> servers;
  for (size_t i = 0; i < rootBlocks.size(); i++) {
    if (rootBlocks[i].getName() == "http") {
      std::vector<BlockParser> serverBlocks = rootBlocks[i].getNestedBlocks();
      for (size_t j = 0; j < serverBlocks.size(); j++) {
        ServerConfig srv = buildServer(serverBlocks[j]);
        servers.push_back(srv);
      }
    }
  }

  return servers;
}