#include "../../includes/cgi/CGIHandler.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

/**
 * @file CGIHandler.cpp
 * @brief CGI request handler - orchestrates complete CGI workflow (Request
 * → Response)
 *
 * This is the main entry point and orchestrator for the entire CGI module.
 * It coordinates all 5 CGI submodules to process a request and generate a
 * response:
 *
 * Pipeline architecture:
 *   Request (HTTP)
 *       ↓
 *   [1. CGIDetector] ────→ Detect CGI + resolve paths
 *       ↓
 *   [2. CGIEnvironment] ─→ Prepare ~20 environment variables
 *       ↓
 *   [3. CGIExecutor] ────→ Fork + Exec + Pipes
 *       ↓
 *   [4. CGIOutputParser] → Parse headers/body
 *       ↓
 *   [5. Response Builder] → Construct HTTP response
 *       ↓
 *   Response (HTTP)
 *
 * Error handling strategy:
 * - Non-CGI requests: Return 404 (resource not found)
 * - Missing executable: Return 404 (CGI not configured)
 * - Fork/exec failures: Return 500 (internal server error)
 * - Memory cleanup: freeEnvArray() in ALL exit paths (success + error)
 *
 * Responsibilities:
 * - Validate request is CGI-eligible
 * - Orchestrate all submodule calls in correct order
 * - Handle errors gracefully (no crashes, appropriate HTTP codes)
 * - Ensure no memory leaks (cleanup in try and catch blocks)
 * - Bridge Request/LocationConfig → Response
 *
 * Integration with webserver:
 * This class is the ONLY public interface the webserver needs to call.
 * All other CGI modules are internal implementation details.
 *
 * Usage example (in webserver):
 *   CGIHandler cgiHandler;
 *   Response response = cgiHandler.handle(request, locationConfig);
 *   // response ready to send to client
 *
 * @note Stateless design - handle() contains no side effects on class
 * members
 * @see handle() method for complete workflow documentation
 */

/**
 * @brief Default constructor
 *
 * Initializes a CGIHandler object. The class is stateless (no member
 * variables to initialize), so the constructor is empty.
 *
 * Design rationale:
 * - All state is local to handle() method (request-scoped)
 * - No shared state between requests (thread-safe by design)
 * - Can reuse same CGIHandler instance for multiple requests
 *
 * @note Constructor is empty because CGIHandler is stateless
 */
CGIHandler::CGIHandler() {}

/**
 * @brief Destructor
 *
 * Cleans up the CGIHandler object. No cleanup needed because the class
 * has no member variables and all resources are managed within handle().
 *
 * @note Destructor is empty because CGIHandler has no owned resources
 */
CGIHandler::~CGIHandler() {}

/**
 * @brief Handles a CGI request from start to finish (main orchestrator)
 *
 * This is the complete CGI processing pipeline. It takes an HTTP request and
 * location configuration, executes the appropriate CGI script, and returns
 * an HTTP response.
 *
 * === WORKFLOW (6 PHASES) ===
 *
 * PHASE 1: CGI DETECTION
 *   Purpose: Verify request should be handled as CGI
 *   Module: CGIDetector::isCGIRequest()
 *   Input: URI + configured extensions
 *   Logic: Extract file extension, compare with cgiExts
 *   Success: Continue to phase 2
 *   Failure: Return 404 "Not Found"
 *
 *   Example:
 *     URI = "/script.php?id=1"
 *     cgiExts = {".php", ".py"}
 *     → Extension ".php" matches → isCGI = true
 *
 * PHASE 2: PATH RESOLUTION
 *   Purpose: Build complete filesystem path to script
 *   Module: CGIDetector::resolveScriptPath()
 *   Input: URI + document root
 *   Logic: Combine root + clean URI (handles trailing slashes)
 *   Result: scriptPath = "./test_scripts/hello.php"
 *
 * PHASE 3: EXECUTABLE LOOKUP
 *   Purpose: Find CGI interpreter for script
 *   Module: CGIDetector::getCGIExecutable()
 *   Input: scriptPath + cgiPaths + cgiExts
 *   Logic: Match extension to interpreter
 *   Success: executable = "/usr/bin/php-cgi"
 *   Failure: executable.empty() → Return 404
 *
 * PHASE 4: ENVIRONMENT PREPARATION
 *   Purpose: Prepare CGI environment variables (RFC 3875)
 *   Module: CGIEnvironment::prepare()
 *   Input: Request + paths + server config
 *   Output: ~20 environment variables
 *   Conversion: toEnvArray() → char** for execve
 *
 *   Variables prepared:
 *     GATEWAY_INTERFACE, SERVER_SOFTWARE, SERVER_PROTOCOL,
 *     SERVER_NAME, SERVER_PORT, REQUEST_METHOD, QUERY_STRING,
 *     SCRIPT_NAME, SCRIPT_FILENAME, CONTENT_TYPE, CONTENT_LENGTH,
 *     HTTP_* (all request headers)
 *
 * PHASE 5: SCRIPT EXECUTION (try-catch protected)
 *   Purpose: Execute CGI script and capture output
 *   Module: CGIExecutor::execute()
 *   Process: fork() → dup2() → execve() → waitpid()
 *   Input: executable path, script path, envp, request body
 *   Output: Raw CGI output (headers + body)
 *   Errors: Caught by exception handler → 500 response
 *
 * PHASE 6: OUTPUT PARSING
 *   Purpose: Split headers/body, extract status code
 *   Module: CGIOutputParser::parse()
 *   Input: Raw output string
 *   Output: Structured headers map + body string + status code
 *   Response: Construct Response object with parsed data
 *
 * === ERROR HANDLING ===
 *
 * 404 "Not Found" returned when:
 * - Request is not CGI (extension doesn't match cgiExts)
 * - CGI executable not found/configured for extension
 *
 * 500 "Internal Server Error" returned when:
 * - fork() fails (process creation error)
 * - execve() fails (interpreter not found/permissions)
 * - Any exception thrown during execution phase
 *
 * === MEMORY MANAGEMENT ===
 *
 * Critical: envp array MUST be freed in ALL exit paths
 *
 * Success path:
 *   1. toEnvArray() allocates envp
 *   2. execute() uses envp (child gets copy)
 *   3. parse() extracts data
 *   4. freeEnvArray(envp) before return
 *
 * Error path (catch block):
 *   1. Exception thrown during execute()
 *   2. catch block: freeEnvArray(envp) BEFORE return 500
 *   3. Prevents memory leak on error
 *
 * Why manual memory management:
 * - execve() requires char** (POSIX interface)
 * - No RAII wrapper possible (char** is raw C interface)
 * - Explicit cleanup guarantees no leaks
 *
 * === INTEGRATION NOTES ===
 *
 * LocationConfig requirements:
 * - root: Document root path (e.g., "./test_scripts")
 * - cgiPaths: List of interpreter paths {"/usr/bin/php-cgi",
 * "/usr/bin/python3"}
 * - cgiExts: List of extensions {".php", ".py", ".sh"}
 * - serverName: Server name from config ("localhost")
 * - serverPort: Listen port (8080)
 *
 * Request requirements (minimum interface):
 * - getURI(): Full URI with query string
 * - getMethod(): HTTP method (GET, POST, etc.)
 * - getHeaders(): Map of all HTTP headers
 * - getBody(): Request body content
 * - getHeader(key): Single header accessor
 * - getProtocol(): HTTP version string
 *
 * Response interface (current):
 * - Response(statusCode, body): Construct with code and body
 * - Future: May need setHeader() for custom headers from CGI
 *
 * === EXAMPLES ===
 *
 * Example 1: Successful PHP execution
 *   Request: GET /hello.php?name=world
 *   Location: cgiExts={".php"}, cgiPaths={"/usr/bin/php-cgi"}
 *   Flow:
 *     1. isCGIRequest → true (matches .php)
 *     2. scriptPath = "./test_scripts/hello.php"
 *     3. executable = "/usr/bin/php-cgi"
 *     4. env.prepare() → 20 variables
 *     5. executor.execute() → "<html>Hello world</html>"
 *     6. parser.parse() → statusCode=200, body="<html>..."
 *     7. Return Response(200, "<html>...")
 *
 * Example 2: Non-CGI request
 *   Request: GET /index.html
 *   Location: cgiExts={".php"}
 *   Flow:
 *     1. isCGIRequest → false (no .php extension)
 *     2. Return Response(404, "Not Found")
 *
 * Example 3: Fork failure
 *   Request: GET /script.php
 *   Condition: System out of processes
 *   Flow:
 *     1-4. Normal
 *     5. executor.execute() throws exception
 *     6. Catch block: freeEnvArray(envp)
 *     7. Return Response(500, "Internal Server Error")
 *
 * @param req HTTP request object (contains method, URI, headers, body)
 * @param location Location configuration (root, CGI paths/exts, server info)
 * @return HTTP response with appropriate status code and body
 *
 * @note This is the ONLY public method webserver needs to call
 * @note All exceptions are caught - never throws to caller
 * @note Memory is always cleaned up (envp freed in all paths)
 * @see RFC 3875 for CGI/1.1 specification
 */
HttpResponse CGIHandler::handle(const HttpRequest &request,
                                const LocationConfig &location,
                                const std::string &serverName, int serverPort) {
  // PHASE 1: Detect if request should be handled as CGI
  // Use getPath() because isCGIRequest checks extension on the path
  bool isCGI =
      CGIDetector::isCGIRequest(request.getPath(), location.getCgiExts());

  if (!isCGI) {
    std::cerr << "❌ [Error] Request path is not a CGI script: "
              << request.getPath() << std::endl;
    HttpResponse response;
    response.setErrorResponse(404);
    return response;
  }
  // PHASE 2: Resolve complete filesystem path to script
  std::string scriptPath =
      CGIDetector::resolveScriptPath(request.getPath(), location.getRoot());
  // PHASE 3: Find the interpreter executable (python3, bash, etc.)
  std::string executable = CGIDetector::getCGIExecutable(
      scriptPath, location.getCgiPaths(), location.getCgiExts());

  // Security check: No interpreter configured OR script doesn't exist on disk
  if (executable.empty() || access(scriptPath.c_str(), F_OK) != 0) {
    std::cerr << "❌ [Error] CGI executable not found or script not accessible: "
              << scriptPath << std::endl;
    HttpResponse response;
    response.setErrorResponse(404); // Resource not executable or doesn't exist
    return response;
  }
  // PHASE 4: Prepare CGI environment variables
  // request.getPath() is already the path without query string (HttpRequest
  // separates them)
  std::string scriptName = request.getPath();
  CGIEnvironment env;
  env.prepare(request, scriptPath, scriptName, serverName, serverPort);

  char **envp = env.toEnvArray();
  // PHASE 5 & 6: Execute script and parse output (error-protected)
  try {
    // PHASE 5: Execute CGI script via fork/exec/pipes
    CGIExecutor executor;
    std::string output =
        executor.execute(executable, scriptPath, envp, request.getBody());
    // PHASE 6: Parse raw output into headers and body
    CGIOutputParser parser;
    parser.parse(output);

    // Build HTTP response from parsed CGI output
    HttpResponse response;

    // 1. Status
    response.setStatus(
        parser.getStatusCode(),
        HttpResponse::getHttpStatusMessage(parser.getStatusCode()));

    // 2. Body
    response.setBody(parser.getBody());

    // 3. Headers del CGI
    std::map<std::string, std::string> cgiHeaders = parser.getHeaders();
    for (std::map<std::string, std::string>::iterator it = cgiHeaders.begin();
         it != cgiHeaders.end(); ++it) {
      if (toUpperCase(it->first) != "STATUS") // Status is set via setStatus
        response.setHeader(it->first, it->second);
    }

    // 4. Cookies del CGI
    std::vector<std::string> cgiCookies = parser.getSetCookies();
    for (size_t i = 0; i < cgiCookies.size(); ++i) {
      response.setCookie(cgiCookies[i]);
    }

    // Cleanup: Free environment array (success path)
    env.freeEnvArray(envp);
    return response;
  } catch (std::exception &e) {
    // Exception during CGI execution (fork/execve failed, script error, etc.)
    // Return 500 Internal Server Error and ensure memory cleanup
    std::cerr << "❌ [Error] CGI execution failed: " << e.what() << std::endl;
    env.freeEnvArray(envp);
    HttpResponse response;
    response.setErrorResponse(500);
    return response;
  }
}

/**
 * @brief Async version of handle - forks CGI but doesn't wait
 *
 * Same as handle() but uses executeAsync() instead of execute().
 * Returns pipe FD and PID so caller can add to poll() and read later.
 */
CGIAsyncResult CGIHandler::handleAsync(const HttpRequest &request,
                                       const LocationConfig &location,
                                       const std::string &serverName,
                                       int serverPort) {
  CGIAsyncResult failResult;
  failResult.pipeFd = -1;
  failResult.childPid = 0;
  failResult.success = false;

  // PHASE 1: Detect if request should be handled as CGI
  CGIDetector detector;
  if (!detector.isCGIRequest(request.getPath(), location.getCgiExts())) {
    return failResult;
  }

  // PHASE 2: Resolve script path (using CGIDetector static method)
  std::string scriptPath =
      CGIDetector::resolveScriptPath(request.getPath(), location.getRoot());

  // PHASE 3: Determine executable (using CGIDetector static method)
  std::string executable = CGIDetector::getCGIExecutable(
      scriptPath, location.getCgiPaths(), location.getCgiExts());

  // Security check: No interpreter configured OR script doesn't exist on disk
  if (executable.empty() || access(scriptPath.c_str(), F_OK) != 0) {
    std::cerr << "❌ [Error] CGI script not found: " << scriptPath << std::endl;
    return failResult; // Caller should return 404
  }

  // PHASE 4: Build environment
  CGIEnvironment env;
  // scriptName es el path URL del script (ej: /cgi-bin/test.py)
  std::string scriptName = request.getPath();
  env.prepare(request, scriptPath, scriptName, serverName, serverPort);
  char **envp = env.toEnvArray();

  // PHASE 5: Execute async (fork but don't wait)
  CGIExecutor executor;
  CGIAsyncResult result =
      executor.executeAsync(executable, scriptPath, envp, request.getBody());

  // Free environment array immediately - child has its own copy
  env.freeEnvArray(envp);

  return result;
}

/**
 * @brief Build HTTP response from completed CGI output buffer
 *
 * Used after readCGIOutput() has collected all the CGI output.
 * Parses headers and body, builds proper HttpResponse.
 */
HttpResponse
CGIHandler::buildResponseFromCGIOutput(const std::string &cgiOutput) {
  HttpResponse response;

  if (cgiOutput.empty()) {
    std::cerr << "❌ [Error] CGI output is empty" << std::endl;
    response.setErrorResponse(500);
    return response;
  }

  CGIOutputParser parser;
  parser.parse(cgiOutput);

  // 1. Status (use "OK" as default message since parser doesn't provide it)
  response.setStatus(parser.getStatusCode(), "OK");

  // 2. Body
  response.setBody(parser.getBody());

  // 3. Headers
  std::map<std::string, std::string> cgiHeaders = parser.getHeaders();
  for (std::map<std::string, std::string>::iterator it = cgiHeaders.begin();
       it != cgiHeaders.end(); ++it) {
    if (toUpperCase(it->first) != "STATUS")
      response.setHeader(it->first, it->second);
  }

  // 4. Cookies
  std::vector<std::string> cgiCookies = parser.getSetCookies();
  for (size_t i = 0; i < cgiCookies.size(); ++i) {
    response.setCookie(cgiCookies[i]);
  }

  return response;
}
