#include "http/RequestHandler.hpp"
#include "cgi/CGIDetector.hpp"
#include "cgi/CGIHandler.hpp"
#include "network/ClientConnection.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

/**
 * @file RequestHandler.cpp
 * @brief HTTP request orchestration and routing
 *
 * This is the central request processing module. It orchestrates:
 * 1. Virtual host selection based on Host header
 * 2. Location matching (longest prefix match)
 * 3. Method validation
 * 4. Body size limit enforcement
 * 5. Redirect handling
 * 6. CGI detection and execution
 * 7. Static file serving
 * 8. Error page handling
 *
 * @see StaticFileHandler for file serving
 * @see CGIHandler for CGI execution
 * @see ServerConfig for virtual host configuration
 * @see LocationConfig for location rules
 */

/**
 * @brief Default constructor
 */
RequestHandler::RequestHandler() {}

/**
 * @brief Destructor
 */
RequestHandler::~RequestHandler() {}

/**
 * @brief Main request handling function
 *
 * Flow:
 * 1. Check for malformed request → 400
 * 2. Match virtual host (ServerConfig) by Host header
 * 3. Match location (longest prefix match)
 * 4. Check if method is allowed → 405
 * 5. Check body size limit → 413
 * 6. Handle redirects (return directive)
 * 7. Detect and execute CGI if applicable
 * 8. Otherwise delegate to StaticFileHandler
 * 9. Apply custom error pages if needed
 *
 * @param request Parsed HTTP request
 * @param candidateConfigs Server configs for this port
 * @param client Client connection (for async CGI), may be NULL
 * @return HttpResponse ready to send
 */
HttpResponse
RequestHandler::handleRequest(const HttpRequest &request,
                              const std::vector<ServerConfig> &candidateConfigs,
                              ClientConnection *client) {
  HttpResponse response;

  // Step 1: Check for malformed request
  if (request.isMalformed()) {
    std::cout << "[Info] Malformed request detected → 400" << std::endl;
    response.setErrorResponse(400);
    return response;
  }

  // Step 2: Virtual host matching
  const ServerConfig *matchedConfig =
      _matchVirtualHost(request, candidateConfigs);
  if (!matchedConfig) {
    std::cerr << "❌ [Error] No matching virtual host for: " << request.getPath()
              << std::endl;
    response.setErrorResponse(500);
    return response;
  }

  // Step 3: Location matching
  const LocationConfig *matchedLocation =
      _matchLocation(request.getPath(), *matchedConfig);
  if (!matchedLocation) {
    std::cout << "[Debug] No location matched → 404" << std::endl;
    _sendError(404, response, *matchedConfig, request);
    return response;
  }

  const LocationConfig &location = *matchedLocation;

  // Step 4: Method validation
  const std::string &method = request.getMethod();
  if (!location.isMethodAllowed(method)) {
    _sendError(405, response, *matchedConfig, request, &location);
    return response;
  }

  // Step 5: Body size limit
  if (request.getBody().size() > location.getMaxBodySize()) {
    _sendError(413, response, *matchedConfig, request, &location);
    return response;
  }

  // Step 6: Redirects
  if (location.getReturnCode() != 0) {
    response.setStatus(location.getReturnCode(), "Redirect");
    response.setHeader("Location", location.getReturnUrl());
    _applyConnectionHeader(request, response);
    return response;
  }

  // Step 7: CGI detection and execution
  if (CGIDetector::isCGIRequest(request.getPath(), location.getCgiExts())) {
    CGIHandler cgiHandler;

    // Check if script file exists BEFORE attempting execution
    std::string scriptPath =
        CGIDetector::resolveScriptPath(request.getPath(), location.getRoot());
    if (access(scriptPath.c_str(), F_OK) != 0) {
      std::cout << "⚠️ [Warning] CGI script not found: " << scriptPath
                << std::endl;
      _sendError(404, response, *matchedConfig, request, &location);
      return response;
    }

    // Extract server name from Host header
    std::string serverName = request.getOneHeader("Host");
    size_t colonPos = serverName.find(':');
    if (colonPos != std::string::npos)
      serverName = serverName.substr(0, colonPos);

    if (serverName.empty() && !matchedConfig->getServerNames().empty()) {
      serverName = matchedConfig->getServerNames()[0];
    }
    int serverPort = matchedConfig->getListen();

    // Async CGI execution path
    if (client) {
      CGIAsyncResult asyncResult =
          cgiHandler.handleAsync(request, location, serverName, serverPort);

      if (asyncResult.success) {
        client->startCGI(asyncResult.pipeFd, asyncResult.childPid);
        response.setCGIPending(true);
        return response;
      } else {
        std::cerr << "❌ [Error] CGI async execution failed" << std::endl;
        _sendError(500, response, *matchedConfig, request, &location);
        _applyConnectionHeader(request, response);
        return response;
      }
    }

    // Fallback: sync execution (for internal tests)
    response = cgiHandler.handle(request, location, serverName, serverPort);
    _applyConnectionHeader(request, response);
    return response;
  }

  // Step 8: Static file handling
  if (method == "GET") {
    _staticHandler.handleGet(request, response, location);
  } else if (method == "HEAD") {
    _staticHandler.handleHead(request, response, location);
  } else if (method == "POST") {
    _staticHandler.handlePost(request, response, location);
  } else if (method == "DELETE") {
    _staticHandler.handleDelete(request, response, location);
  } else {
    _sendError(405, response, *matchedConfig, request, &location);
  }

  // Step 9: Apply custom error pages if needed
  if (response.getStatusCode() >= 400) {
    _sendError(response.getStatusCode(), response, *matchedConfig, request,
               &location);
  }

  _applyConnectionHeader(request, response);
  return response;
}

/**
 * @brief Matches virtual host based on Host header
 *
 * Searches candidate configs for one whose server_name matches
 * the Host header. Falls back to first config if no match.
 *
 * @param request HTTP request with Host header
 * @param candidateConfigs Configs listening on this port
 * @return Matched ServerConfig pointer, or NULL if empty
 */
const ServerConfig *RequestHandler::_matchVirtualHost(
    const HttpRequest &request,
    const std::vector<ServerConfig> &candidateConfigs) {
  if (candidateConfigs.empty())
    return NULL;

  std::string host = request.getOneHeader("Host");
  size_t colonPos = host.find(':');
  if (colonPos != std::string::npos)
    host = host.substr(0, colonPos);

  for (size_t i = 0; i < candidateConfigs.size(); ++i) {
    const std::vector<std::string> &serverNames =
        candidateConfigs[i].getServerNames();
    for (size_t j = 0; j < serverNames.size(); ++j) {
      if (serverNames[j] == host) {
        return &candidateConfigs[i];
      }
    }
  }
  return &candidateConfigs[0]; // Default to first
}

/**
 * @brief Matches location using longest prefix match
 *
 * Iterates through all locations in the config and finds the one
 * with the longest matching prefix for the request path.
 *
 * @param path Request URL path
 * @param config Server configuration
 * @return Matched LocationConfig pointer, or NULL if no match
 */
const LocationConfig *
RequestHandler::_matchLocation(const std::string &path,
                               const ServerConfig &config) {
  const std::vector<LocationConfig> &locations = config.getLocations();
  const LocationConfig *matchedLocation = NULL;
  size_t longestMatch = 0;

  for (size_t i = 0; i < locations.size(); ++i) {
    const std::string &locationPattern = locations[i].getPattern();

    if (path.compare(0, locationPattern.length(), locationPattern) == 0) {
      if (locationPattern.length() > longestMatch) {
        longestMatch = locationPattern.length();
        matchedLocation = &locations[i];
      }
    }
  }
  return matchedLocation;
}

/**
 * @brief Sets Connection header based on keep-alive status
 *
 * @param request HTTP request
 * @param response HTTP response to modify
 */
void RequestHandler::_applyConnectionHeader(const HttpRequest &request,
                                            HttpResponse &response) {
  if (request.isKeepAlive())
    response.setHeader("Connection", "keep-alive");
  else
    response.setHeader("Connection", "close");
}

/**
 * @brief Generates error response with custom page support
 *
 * Tries to load custom error page from:
 * 1. Location error_page directive
 * 2. Server error_page directive
 * 3. Falls back to built-in styled error page
 *
 * @param errorCode HTTP error code
 * @param response Response to populate
 * @param config Server configuration for error pages
 * @param request Original request
 * @param location Current location (may be NULL)
 */
void RequestHandler::_sendError(int errorCode, HttpResponse &response,
                                const ServerConfig &config,
                                const HttpRequest &request,
                                const LocationConfig *location) {
  std::string errorPagePath;
  std::string rootUsed;

  // Priority 1: Location-level error page
  if (location) {
    const std::map<int, std::string> &locErrorPages = location->getErrorPages();
    std::map<int, std::string>::const_iterator it =
        locErrorPages.find(errorCode);
    if (it != locErrorPages.end()) {
      errorPagePath = it->second;
      rootUsed = location->getRoot();
    }
  }

  // Priority 2: Server-level error page
  if (errorPagePath.empty()) {
    const std::map<int, std::string> &servErrorPages = config.getErrorPages();
    std::map<int, std::string>::const_iterator it =
        servErrorPages.find(errorCode);
    if (it != servErrorPages.end()) {
      errorPagePath = it->second;
      rootUsed = config.getRoot();
    }
  }

  // Try to load custom error page
  if (!errorPagePath.empty()) {
    if (rootUsed.empty())
      rootUsed = ".";
    if (rootUsed[rootUsed.size() - 1] == '/')
      rootUsed.erase(rootUsed.size() - 1);

    std::string separator = (errorPagePath[0] == '/') ? "" : "/";
    std::string fullPath = rootUsed + separator + errorPagePath;

    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
      std::ifstream file(fullPath.c_str());
      if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();

        response.setStatus(errorCode, "Error");
        response.setHeader("Content-Type", "text/html");
        response.setBody(buffer.str());
        _applyConnectionHeader(request, response);
        return;
      }
    }
  }

  // Fallback: built-in error page
  if (errorCode >= 400) {
    std::cerr << "⚠️ [Warning] Error " << errorCode
              << " for: " << request.getPath() << std::endl;
  }
  response.setErrorResponse(errorCode);
  _applyConnectionHeader(request, response);
}
