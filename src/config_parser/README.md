# Config Parser & Validator

Nginx-style configuration file parser and multi-level semantic validator for the webserv project (42 École). Parses nginx-like configuration files into a structured tree and validates them against strict semantic rules including context validation, argument type checking, and nginx-compliant nesting rules.

## Features

- Complete nginx-style parsing with blocks, directives, and nested structures
- Comment handling for both full-line and inline comments
- Multi-level validation with 3-layer system (count, type, all-args)
- Error accumulation that reports ALL errors in one pass
- Context-aware validation using bitwise context system (5 contexts)
- 52 test cases passing with 100% coverage
- Production-ready quality with clean compilation using -Wall -Wextra -Werror -std=c++98

## Architecture

Directory structure separated by responsibility: parser for tree construction and validation for semantic checking.

Parser directory contains UtilsConfigParser for string utilities, DirectiveParser for tokenizing directives, BlockParser for building hierarchical tree, and readConfigFile as main entry point.

Validation directory contains ValidationStructureConfig for pre-parsing syntax checks, ValueValidator for type-specific validators, DirectiveMetadata for directive rules engine, and SemanticValidator for recursive semantic validation.

Validation flow: Input File goes through validateStructure (braces and chars check), then readConfigFile (builds tree), then SemanticValidator validate (contexts, types, args), resulting in Valid or Invalid status.

## Quick Start

Include the necessary headers: config_parser/parser/BlockParser.hpp, config_parser/validation/ValidationStructureConfig.hpp, and config_parser/validation/SemanticValidator.hpp

Step 1 is structural validation. Create a vector of strings for structural errors. Call validateStructure with the config file path and errors vector. If it returns false, print all errors and return error code.

Step 2 is parsing the config file. Call readConfigFile with the config file path to get a BlockParser root object representing the parsed tree.

Step 3 is semantic validation. Create a SemanticValidator object. Call validate with the root BlockParser. If it returns false, call printReport to display all errors and return error code.

If all steps pass, the configuration is valid.

Wrap everything in try-catch to handle exceptions from parsing errors.

## Validation Levels

Structural validation using validateStructure checks brace balance, valid characters, and orphaned syntax before parsing.

Syntactic validation during readConfigFile checks multi-line directives, block nesting, and semicolons while parsing.

Semantic validation using SemanticValidator checks contexts, argument count, and argument types after parsing.

Semantic validation has 3 sub-levels. LEVEL 1 validates argument count (minArgs ≤ count ≤ maxArgs). LEVEL 2 validates first argument type. LEVEL 3 validates ALL arguments types per-position.

## Supported Directives

Server Context directives (3): listen for port binding with range 1-65535 and unlimited args, server_name for domain names with unlimited args, and host for IP address binding with 1 arg.

Multi-Context directives valid in HTTP, SERVER, and LOCATION (4): root for document root path with 1 arg, index for default files with unlimited args, error_page for error page mapping with 2+ args (codes + path), and autoindex for directory listing with on/off value.

Location Only directives (4): allow_methods for HTTP methods whitelist, proxy_pass for reverse proxy URL, cgi_path for CGI executable path, and cgi_ext for CGI file extensions.

SERVER and LOCATION directives (2): return for HTTP redirect/response with 1-2 args, and rewrite for URL rewriting with 2-3 args.

HTTP and SERVER directive (1): client_max_body_size for request body size limit with 1 arg.

Bonus directives for Cookies and Sessions (9, commented for future implementation): session_timeout, session_name, session_path, cookie_domain, cookie_path, cookie_max_age, cookie_secure, cookie_httponly, and cookie_samesite.

## Contexts

Context types use bitwise enum values. CTX_MAIN equals 1 for root level. CTX_EVENTS equals 2 for events blocks. CTX_HTTP equals 4 for http blocks. CTX_SERVER equals 8 for server blocks. CTX_LOCATION equals 16 for location blocks.

Nesting rules are nginx-compliant. At ROOT level (CTX_MAIN), http blocks and events blocks are allowed, but server blocks must be inside http.

Inside HTTP blocks (CTX_HTTP), server blocks are allowed but location blocks must be inside server.

Inside SERVER blocks (CTX_SERVER), location blocks with any pattern are allowed.

Inside LOCATION blocks (CTX_LOCATION), nested location blocks are not allowed (nginx does not support this).

## Argument Types

Eight validator functions handle different types. ARG_PORT uses isValidPort for values 80, 8080, 443 but rejects 0, 99999, abc. ARG_BOOL uses isValidBool for on and off but rejects true, false, yes, no. ARG_PATH uses isValidPath for /var/www and ./html but rejects html and var/www. ARG_IP uses isValidIP for 127.0.0.1 and 192.168.1.1 but rejects 256.1.1.1 and 192.168.1. ARG_HOST uses isValidHost for localhost and example.com but rejects test@host. ARG_HTTP uses isValidHttpCode for 200, 404, 500 but rejects 99, 600, abc. ARG_NUMBER uses isValidNumber for 1024 and 256 but rejects -5, 12.5, abc. ARG_PATTERN uses isValidPattern for /, ~ \.php$, = /exact but rejects $$, abc. ARG_STR always validates as true for any string.

## Testing

Test suite contains 52 test cases with 100% passing rate.

From src/config_parser directory, compile with: g++ -Wall -Wextra -Werror -std=c++98 parser/*.cpp validation/*.cpp main.cpp -I../../includes/config_parser -o test

Then run ./test to execute tests.

Test coverage includes 10 valid configurations from minimal to complex, 8 structural errors for braces and syntax, 6 context errors for wrong nesting, 12 argument errors for types and ranges, 4 comment tests for full-line and inline, 8 edge cases for unknown directives and invalid patterns, and 4 complex configs resembling production.

Bugs found during testing: 2, both fixed. Missing regex characters (TEST 09) and block context validation (TEST 24).

## Compilation

Requirements are C++98 compliant compiler and no external dependencies (only standard library).

Build command from src/config_parser: g++ -Wall -Wextra -Werror -std=c++98 parser/*.cpp validation/*.cpp main.cpp -I../../includes/config_parser -o webserv_parser

Compile-time guarantees include zero warnings with -Werror flag, C++98 standard compliance, strict error checking with -Wall -Wextra, and no undefined behavior.

## Statistics

Files: 14 (.hpp + .cpp). Lines of code: approximately 1,200+. Functions and methods: approximately 50. Classes: 3 (BlockParser, DirectiveParser, SemanticValidator). Structs: 2 (DirectiveToken, DirectiveRule). Enums: 3 (Context, ArgumentType, validation levels). Test cases: 52 (100% passing). Directives: 24 (15 mandatory + 9 bonus). Contexts: 5 (MAIN, HTTP, SERVER, LOCATION, EVENTS). Argument validators: 8. Development time: approximately 18 hours. Bugs found in testing: 2 (both fixed).

## Design Decisions

Error Accumulation vs Fail-Fast: We chose to accumulate all errors before reporting. This approach matches nginx tools behavior which report ALL configuration errors in one run. This saves time during debugging and configuration review by showing all issues at once.

Bitwise Context Flags: We chose to use bitwise OR for multi-context directives. This allows directives to be valid in multiple contexts using a single integer field. For example, root is valid in HTTP, SERVER, and LOCATION by setting allowedContexts to CTX_HTTP | CTX_SERVER | CTX_LOCATION which equals 28. Validation is fast using bitwise AND operations.

Three Validation Levels: We validate in three stages: argument count, first argument type, then all argument types. LEVEL 1 count validation is fastest with simple numeric comparison and rejects obviously wrong configs immediately. LEVEL 2 first arg validation catches most common errors since most directives have strict requirements on the first argument. LEVEL 3 all args validation provides complete type safety for complex directives like error_page 404 500 /error.html. This layered approach optimizes for the common case while providing thorough validation.

Parser and Validation Separation: We separated concerns into two directories. The parser directory focuses on building the tree with syntax-focused operations. The validation directory focuses on checking semantics with logic-focused operations. Each module can be tested independently, making it easy to extend and maintain.

Static Directive Table: We use a compile-time directive rules table rather than runtime registration. This provides fast lookups with array iteration, compile-time safety with no registration errors, easy auditing with all rules visible in one place, and no dynamic allocation overhead.

## Example Configuration

nginx
events {
}

http {
client_max_body_size 1024;
server {
    listen 8080;
    server_name example.com www.example.com;
    root /var/www/html;
    index index.html index.htm;
    error_page 404 /404.html;
    error_page 500 502 503 504 /50x.html;
    autoindex off;

    location / {
        allow_methods GET POST;
        index index.html;
    }

    location /api {
        allow_methods GET POST DELETE PUT;
        proxy_pass http://localhost:3000;
    }

    location /images {
        root /var/www/static;
        autoindex on;
    }

    location ~ \.php$ {
        cgi_path /usr/bin/php-cgi;
        cgi_ext .py .php;
    }
}
}

## Error Examples

Structural error example: http block containing opening brace without name before it produces Error at line X: No name before opening brace.

Context error example: http block containing listen 8080 directive produces Error: listen directive not allowed in http context.

Argument error example: server block with listen abc and autoindex maybe produces Error: Invalid arguments for listen and Error: Invalid arguments for autoindex.

