#include "../../includes/cgi/CGIEnvironment.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

/**
 * @file CGIEnvironment.cpp
 * @brief CGI environment variable preparation and management (RFC 3875
 * compliant)
 *
 * This module prepares all environment variables required by CGI scripts
 * according to RFC 3875 (Common Gateway Interface specification). It
 * handles approximately 20+ variables including:
 * - Server metadata (GATEWAY_INTERFACE, SERVER_SOFTWARE, SERVER_PROTOCOL)
 * - Server configuration (SERVER_NAME, SERVER_PORT)
 * - Request metadata (REQUEST_METHOD, QUERY_STRING, CONTENT_TYPE,
 * CONTENT_LENGTH)
 * - Script information (SCRIPT_NAME, SCRIPT_FILENAME)
 * - HTTP headers converted to HTTP_* format (HTTP_HOST, HTTP_USER_AGENT,
 * etc.)
 *
 * Memory management:
 * The class converts std::map to char** for execve() system call
 * compatibility. Memory is manually allocated with new/delete (C++98 style,
 * not malloc/free) and must be explicitly freed after execve() to prevent
 * leaks.
 *
 * Key design decisions:
 * - Uses std::map for easy access and extension
 * - Separates preparation (prepare) from conversion (toEnvArray)
 * - Manual memory management required for POSIX execve() interface
 * - REDIRECT_STATUS added for php-cgi security requirements
 *
 * @note All environment variables follow RFC 3875 naming conventions
 * @see RFC 3875 section 4.1 (Request Meta-Variables)
 */

/**
 * @brief Default constructor
 *
 * Initializes an empty CGIEnvironment object. The _envVars map is
 * automatically initialized by std::map default constructor (empty map).
 *
 * Environment variables are populated later via prepare() method, which
 * must be called before toEnvArray().
 *
 * @note Constructor is empty because std::map handles its own
 * initialization
 */
CGIEnvironment::CGIEnvironment() {}

/**
 * @brief Destructor
 *
 * Cleans up the CGIEnvironment object. The _envVars std::map is automatically
 * destroyed by its own destructor (RAII principle).
 *
 * Important: This destructor does NOT free the char** array created by
 * toEnvArray(). That memory must be freed explicitly using freeEnvArray() after
 * execve() call.
 *
 * @warning Caller must call freeEnvArray() on any char** created by
 * toEnvArray()
 * @note Destructor is empty because std::map handles its own cleanup
 */
CGIEnvironment::~CGIEnvironment() {}

/**
 * @brief Prepares all CGI environment variables from request and server
 * configuration
 *
 * Populates _envVars map with approximately 20 environment variables required
 * by CGI scripts according to RFC 3875. Variables are grouped into categories:
 *
 * === CONSTANT VARIABLES (CGI specification) ===
 * - GATEWAY_INTERFACE = "CGI/1.1"    → CGI version implemented
 * - SERVER_SOFTWARE = "webserv/1.0"  → Web server identification
 * - SERVER_PROTOCOL = "HTTP/1.1"     → HTTP protocol version
 * - REDIRECT_STATUS = "200"          → PHP-CGI security requirement
 *
 * === SERVER CONFIGURATION (from config file) ===
 * - SERVER_NAME → server_name directive (e.g., "localhost")
 * - SERVER_PORT → listen port (e.g., "8080")
 *
 * === REQUEST METADATA (from HTTP request) ===
 * - REQUEST_METHOD   → GET, POST, DELETE, etc.
 * - QUERY_STRING     → Everything after '?' in URI
 * - CONTENT_TYPE     → From Content-Type header
 * - CONTENT_LENGTH   → Body size in bytes (as string)
 *
 * === SCRIPT INFORMATION (from path resolution) ===
 * - SCRIPT_NAME      → URI path to script (e.g., "/cgi-bin/script.php")
 * - SCRIPT_FILENAME  → Complete filesystem path (e.g.,
 * "./www/cgi-bin/script.php")
 *
 * === HTTP HEADERS (all request headers) ===
 * All HTTP headers are converted to CGI format via convertHeadersToEnv():
 * - Host: localhost           → HTTP_HOST=localhost
 * - User-Agent: Mozilla/5.0   → HTTP_USER_AGENT=Mozilla/5.0
 * - Accept: text/html         → HTTP_ACCEPT=text/html
 *
 * REDIRECT_STATUS explanation:
 * PHP-CGI requires this variable as a security measure to prevent direct
 * execution. Without it, php-cgi refuses to run with error:
 * "Security Alert! The PHP CGI cannot be accessed directly."
 *
 * Complete example (variables accessible in script):
 *   $_SERVER['REQUEST_METHOD']   = "GET"
 *   $_SERVER['QUERY_STRING']     = "name=world&id=42"
 *   $_SERVER['SERVER_NAME']      = "localhost"
 *   $_SERVER['SERVER_PORT']      = "8080"
 *   $_SERVER['SCRIPT_NAME']      = "/hello.php"
 *   $_SERVER['SCRIPT_FILENAME']  = "./test_scripts/hello.php"
 *   $_SERVER['HTTP_HOST']        = "localhost:8080"
 *   $_SERVER['HTTP_USER_AGENT']  = "curl/7.64.1"
 *
 * @param req Request object containing method, URI, headers, body
 * @param scriptPath Complete filesystem path to script file
 * @param scriptName URI path to script (without query string)
 * @param serverName Server name from configuration (server_name directive)
 * @param serverPort Server port from configuration (listen directive)
 *
 * @note Must be called before toEnvArray()
 * @note convertHeadersToEnv() handles HTTP_ prefix conversion automatically
 * @note Uses intToString() for numeric conversions (C++98 compatible)
 * @see RFC 3875 section 4.1 for complete variable specifications
 */
void CGIEnvironment::prepare(const HttpRequest &request,
                             const std::string &scriptPath,
                             const std::string &scriptName,
                             const std::string &serverName, int serverPort) {
  // Convert all HTTP headers to CGI format (HTTP_* variables)
  std::map<std::string, std::string> httpEnvVars =
      convertHeadersToEnv(request.getHeaders());
  std::map<std::string, std::string>::const_iterator it;
  for (it = httpEnvVars.begin(); it != httpEnvVars.end(); ++it) {
    _envVars[it->first] = it->second;
  }
  // CGI/1.1 specification constants
  _envVars["GATEWAY_INTERFACE"] = "CGI/1.1";
  _envVars["SERVER_SOFTWARE"] = "webserv/1.0";
  _envVars["SERVER_PROTOCOL"] = "HTTP/1.1";
  _envVars["REDIRECT_STATUS"] = "200";
  // Server configuration variables
  _envVars["SERVER_NAME"] = serverName;
  _envVars["SERVER_PORT"] = intToString(serverPort);
  // Request metadata
  _envVars["REQUEST_METHOD"] = request.getMethod();
  _envVars["QUERY_STRING"] =
      request.getQuery(); // HttpRequest already extracts query string
  // Script information
  _envVars["SCRIPT_NAME"] = scriptName;
  _envVars["SCRIPT_FILENAME"] = scriptPath;

  // Manual header lookup since HttpRequest doesn't have getHeader(key)
  std::map<std::string, std::string> headers = request.getHeaders();
  if (headers.count("Content-Type")) {
    _envVars["CONTENT_TYPE"] = headers["Content-Type"];
  }
  _envVars["CONTENT_LENGTH"] = intToString(request.getBody().size());
}

/**
 * @brief Converts environment variables map to char** array for execve()
 *
 * Transforms the _envVars std::map into a NULL-terminated array of C strings
 * in "KEY=VALUE" format, as required by the POSIX execve() system call.
 *
 * Memory allocation process:
 * 1. Count variables → count
 * 2. Allocate array of pointers: new char*[count + 1]
 * 3. For each variable:
 *    a. Create string "KEY=VALUE"
 *    b. Allocate char array: new char[size + 1]
 *    c. Copy string to char array with stringToCString()
 * 4. Set last element to NULL (execve requirement)
 *
 * Resulting format:
 *   char **envp = {
 *       "REQUEST_METHOD=GET",
 *       "SERVER_PORT=8080",
 *       "QUERY_STRING=name=world",
 *       "SCRIPT_NAME=/script.php",
 *       "HTTP_HOST=localhost",
 *       NULL                        ← Terminator (required by execve)
 *   };
 *
 * Memory management:
 * - Each string is individually allocated with new char[]
 * - Array of pointers allocated with new char*[]
 * - Caller MUST call freeEnvArray() after execve() to prevent leaks
 * - Memory persists even if CGIEnvironment object is destroyed
 *
 * Usage example:
 *   CGIEnvironment env;
 *   env.prepare(request, scriptPath, scriptName, serverName, port);
 *   char **envp = env.toEnvArray();
 *   execve("/usr/bin/php-cgi", argv, envp);  // envp passed to child process
 *   env.freeEnvArray(envp);                   // MUST free after execve
 *
 * @return NULL-terminated array of C strings in "KEY=VALUE" format
 *
 * @warning Caller is responsible for freeing returned array with freeEnvArray()
 * @note Uses new/delete (not malloc/free) for C++ consistency
 * @note Array size is _envVars.size() + 1 (for NULL terminator)
 * @see freeEnvArray() for memory cleanup
 */
char **CGIEnvironment::toEnvArray() const {
  size_t count = _envVars.size();
  size_t index = 0;

  char **env = new char *[count + 1];

  std::map<std::string, std::string>::const_iterator it;
  for (it = _envVars.begin(); it != _envVars.end(); ++it) {
    std::string envLine = it->first + "=" + it->second;
    int envLineLength = envLine.size();
    env[index] = new char[envLineLength + 1];
    stringToCString(envLine, env[index]);
    index++;
  }
  env[count] = NULL;
  return env;
}

/**
 * @brief Frees memory allocated by toEnvArray()
 *
 * Deallocates the char** array created by toEnvArray() to prevent memory leaks.
 * Must be called after execve() in the parent process, since the child process
 * will have its own copy of the environment.
 *
 * Deallocation process:
 * 1. Iterate through array until NULL terminator
 * 2. Delete each individual string: delete[] env[i]
 * 3. Delete array of pointers: delete[] env
 *
 * Memory structure being freed:
 *   env[0] → "REQUEST_METHOD=GET\0"      ← delete[] this
 *   env[1] → "SERVER_PORT=8080\0"        ← delete[] this
 *   env[2] → "QUERY_STRING=...\0"        ← delete[] this
 *   ...
 *   env[n] → NULL                        ← stop iteration
 *   env (array itself)                   ← delete[] this
 *
 * Usage pattern:
 *   char **envp = env.toEnvArray();
 *   try {
 *       execve(executable, argv, envp);
 *       // Child process continues, parent continues below
 *   } catch (...) {
 *       env.freeEnvArray(envp);  // Free on error
 *       throw;
 *   }
 *   env.freeEnvArray(envp);      // Free on success
 *
 * @param env NULL-terminated array of C strings created by toEnvArray()
 *
 * @warning Must be called exactly once for each toEnvArray() call
 * @warning Do NOT call on stack-allocated or other memory not from toEnvArray()
 * @note Uses delete[] (matching new[] allocation in toEnvArray)
 */
void CGIEnvironment::freeEnvArray(char **env) const {
  for (int i = 0; env[i]; i++) {
    delete[] env[i];
  }
  delete[] env;
}

/**
 * @brief Retrieves value of a single environment variable (debug helper)
 *
 * Looks up a specific variable in the _envVars map by key. Returns empty
 * string if variable doesn't exist. Used primarily for testing and debugging.
 *
 * Uses const_iterator for const correctness (method is const, cannot modify
 * _envVars). Cannot use operator[] because it's non-const (would insert key if
 * missing).
 *
 * Examples:
 *   getVar("REQUEST_METHOD")  → "GET"
 *   getVar("SERVER_PORT")     → "8080"
 *   getVar("NONEXISTENT")     → ""
 *
 * @param key Variable name to retrieve (e.g., "REQUEST_METHOD", "SERVER_PORT")
 * @return Value of the variable, or empty string if not found
 *
 * @note Returns empty string instead of throwing exception for simplicity
 * @note Uses find() instead of operator[] for const correctness
 */
std::string CGIEnvironment::getVar(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it;
  it = _envVars.find(key);
  if (it != _envVars.end())
    return it->second;
  else
    return "";
}

/**
 * @brief Prints all environment variables to stdout (debug helper)
 *
 * Outputs all prepared environment variables in "KEY = VALUE" format.
 * Useful for verifying that prepare() correctly populated all required
 * variables before passing them to execve().
 *
 * Output example:
 *   === CGI ENVIRONMENT VARIABLES ===
 *   CONTENT_LENGTH = 0
 *   CONTENT_TYPE =
 *   GATEWAY_INTERFACE = CGI/1.1
 *   HTTP_HOST = localhost:8080
 *   HTTP_USER_AGENT = curl/7.64.1
 *   QUERY_STRING = name=world&id=42
 *   REDIRECT_STATUS = 200
 *   REQUEST_METHOD = GET
 *   SCRIPT_FILENAME = ./test_scripts/hello.php
 *   SCRIPT_NAME = /hello.php
 *   SERVER_NAME = localhost
 *   SERVER_PORT = 8080
 *   SERVER_PROTOCOL = HTTP/1.1
 *   SERVER_SOFTWARE = webserv/1.0
 *
 * @note For debugging/testing only - should not be called in production
 * @note Output is alphabetically sorted by std::map (automatically)
 */
void CGIEnvironment::printAll() const {

  std::cout << "=== CGI ENVIRONMENT VARIABLES ===" << std::endl;
  std::map<std::string, std::string>::const_iterator it;
  for (it = _envVars.begin(); it != _envVars.end(); ++it) {
    std::cout << it->first << " = " << it->second << std::endl;
  }
}
