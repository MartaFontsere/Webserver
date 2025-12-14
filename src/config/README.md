# Config Module

## Overview

The **Config module** is responsible for converting the parsed configuration file (represented as a `BlockParser` tree) into structured, typed configuration objects (`ServerConfig` and `LocationConfig`) that can be used by the web server at runtime.

This module acts as a **bridge** between the configuration parser and the rest of the application, transforming generic parsed blocks into usable server and location configurations.

---

## Architecture

### Conversion Pipeline

```
BlockParser (generic tree)
        ↓
ConfigBuilder (extraction & conversion)
        ↓
ServerConfig + LocationConfig (typed structures)
```

### Module Components

1. **ConfigBuilder** (`ConfigBuilder.cpp/hpp`)
   - Orchestrates the conversion from `BlockParser` to config objects
   - Extracts directive values from parsed blocks
   - Handles special directives (error_page, return, autoindex)
   - Builds hierarchical configuration structure

2. **ServerConfig** (`ServerConfig.cpp/hpp`)
   - Stores configuration for a single server block
   - Manages network binding, virtual hosts, and global server settings
   - Contains a vector of `LocationConfig` for request routing

3. **LocationConfig** (`LocationConfig.cpp/hpp`)
   - Stores configuration for a single location block
   - Manages URI pattern matching and location-specific settings
   - Configures static file serving, CGI, uploads, and redirects

4. **UtilsConfig** (`UtilsConfig.cpp/hpp`)
   - Provides utility functions for type conversions
   - Converts strings to appropriate types (int, size_t, bool)
   - C++98 compatible implementations

---

## Server Configuration Structure

A `ServerConfig` object represents a single `server` block in the configuration file:

```nginx
server {
    listen 8080;
    host 127.0.0.1;
    server_name localhost example.com;
    root ./www;
    index index.html index.php;
    client_max_body_size 1048576;
    error_page 404 /404.html;

    location / { ... }
    location /api { ... }
}
```

### ServerConfig Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| `_listen` | `int` | Port number for server binding |
| `_host` | `std::string` | IP address or hostname |
| `_serverNames` | `std::vector<std::string>` | Virtual host names |
| `_root` | `std::string` | Default document root |
| `_index` | `std::vector<std::string>` | Default index files |
| `_errorPages` | `std::map<int, std::string>` | Custom error pages |
| `_clientMaxBodySize` | `size_t` | Maximum request body size |
| `_locations` | `std::vector<LocationConfig>` | Location blocks |

---

## Location Configuration Structure

A `LocationConfig` object represents a single `location` block within a server:

```nginx
location /api {
    root ./www;
    index index.html;
    allow_methods GET POST;
    autoindex on;
    cgi_path /usr/bin/php-cgi;
    cgi_ext .php;
    error_page 404 /404.html;
    return 301 /new-location;
    upload_path ./uploads;
    client_max_body_size 1048576;
}
```

### LocationConfig Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| `_pattern` | `std::string` | URI pattern for matching |
| `_root` | `std::string` | Document root for this location |
| `_index` | `std::vector<std::string>` | Index files |
| `_methods` | `std::vector<std::string>` | Allowed HTTP methods |
| `_autoindex` | `bool` | Enable directory listing |
| `_cgiPaths` | `std::vector<std::string>` | CGI interpreter paths |
| `_cgiExts` | `std::vector<std::string>` | CGI file extensions |
| `_errorPages` | `std::map<int, std::string>` | Custom error pages |
| `_returnCode` | `int` | HTTP redirect status code |
| `_returnUrl` | `std::string` | Redirect URL |
| `_uploadPath` | `std::string` | Upload directory |
| `_bodySize` | `size_t` | Max body size for this location |

---

## ConfigBuilder - Conversion Logic

### Three-Layer Architecture

1. **Generic Helpers** - Extract raw values from BlockParser
   - `getDirectiveValue()` - Get single directive value
   - `getDirectiveValues()` - Get multiple directive values
   - `getDirectiveValueAsInt()` - Get and convert to integer

2. **Specialized Parsers** - Handle complex directives
   - `parseAutoindex()` - Convert "on"/"off" to boolean
   - `parseReturn()` - Extract redirect code and URL
   - `parseErrorPages()` - Build error_page map (code → file)

3. **Builders** - Construct config objects
   - `buildLocation()` - Create LocationConfig from location block
   - `buildServer()` - Create ServerConfig from server block
   - `buildFromBlockParser()` - Build all servers from root block

### Error Handling Strategy

- **Missing directives**: Use defaults (empty strings/vectors, 0, false)
- **Invalid values**: Let semantic validator catch issues
- **Type conversions**: Use UtilsConfig functions for safe conversions

---

## Usage Example

```cpp
#include "ConfigBuilder.hpp"
#include "BlockParser.hpp"

// 1. Parse configuration file
BlockParser root = readConfigFile("config/nginx.conf");

// 2. Build typed configuration objects
ConfigBuilder builder;
std::vector<ServerConfig> servers = builder.buildFromBlockParser(root);

// 3. Use configurations
for (size_t i = 0; i < servers.size(); ++i) {
    ServerConfig& server = servers[i];
    std::cout << "Server listening on port: " << server.getListen() << std::endl;
    
    const std::vector<LocationConfig>& locations = server.getLocations();
    for (size_t j = 0; j < locations.size(); ++j) {
        std::cout << "  Location: " << locations[j].getPattern() << std::endl;
    }
}
```

---

## Integration Points

### Dependencies (Input)
- **config_parser module**: Provides `BlockParser` tree from parsed config file

### Consumers (Output)
- **Server class**: Uses `ServerConfig` for socket binding and virtual host setup
- **RequestRouter**: Uses `LocationConfig` for URI pattern matching
- **CGIHandler**: Uses `LocationConfig` for CGI paths and extensions
- **StaticFileHandler**: Uses `LocationConfig` for root, index, autoindex
- **UploadHandler**: Uses `LocationConfig` for upload path and size limits

---

## Design Decisions

### Why Classes Instead of Structs?
- **Encapsulation**: Private members with public getters/setters
- **Validation-ready**: Can add validation logic in setters
- **Future-proof**: Easy to extend without breaking API

### Why STL Containers?
- **No manual memory management**: Automatic cleanup
- **Standard interface**: Familiar for all C++ developers
- **Exception safety**: RAII guarantees

### Why Const References in Getters?
- **Performance**: Avoid expensive copies of vectors/maps
- **Safety**: Prevents accidental modification of internal state

### Default Values
- **Port**: 0 (must be configured)
- **Body size**: 1MB (1048576 bytes)
- **Autoindex**: false
- **Return code**: 0 (no redirect)
- **Strings/vectors/maps**: Empty

---

## File Structure

```
src/config/
├── ConfigBuilder.cpp       # Conversion orchestration
├── ServerConfig.cpp        # Server block configuration
├── LocationConfig.cpp      # Location block configuration
├── UtilsConfig.cpp         # Utility functions
└── README.md              # This file

includes/config/
├── ConfigBuilder.hpp
├── ServerConfig.hpp
├── LocationConfig.hpp
└── UtilsConfig.hpp
```

---

## Future Enhancements

- [ ] Add configuration validation in setters
- [ ] Implement configuration merging (inheritance from server to location)
- [ ] Add support for nested locations
- [ ] Implement configuration hot-reload
- [ ] Add configuration serialization (to JSON/YAML)

---

## Notes

- **C++98 Compatibility**: All code adheres to C++98 standard (no C++11 features)
- **Memory Safety**: No raw pointers, all memory managed by STL containers
- **Thread Safety**: Not thread-safe by design (single-threaded configuration loading)
- **One-time Use**: Configuration objects are built once at startup, then read-only

---

## See Also

- [config_parser module](../config_parser/README.md) - Configuration file parsing
- [BlockParser](../config_parser/parser/BlockParser.hpp) - Parsed configuration tree
- [SemanticValidator](../config_parser/validation/SemanticValidator.hpp) - Configuration validation
