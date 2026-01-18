#pragma once

#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/StaticFileHandler.hpp"
#include <vector>

class ClientConnection;

/**
 * @brief HTTP request orchestrator - routing, virtual hosts, and dispatch
 */
class RequestHandler {
public:
  RequestHandler();
  ~RequestHandler();

  /**
   * @brief Main entry point for processing an HTTP request
   * @param request The complete HTTP request
   * @param candidateConfigs ServerConfigs matching the port
   * @param client Optional - if provided, CGI runs async
   * @return HttpResponse (or pending if CGI async)
   */
  HttpResponse handleRequest(const HttpRequest &request,
                             const std::vector<ServerConfig> &candidateConfigs,
                             ClientConnection *client = NULL);

private:
  StaticFileHandler _staticHandler;

  const ServerConfig *
  _matchVirtualHost(const HttpRequest &request,
                    const std::vector<ServerConfig> &candidateConfigs);
  const LocationConfig *_matchLocation(const std::string &path,
                                       const ServerConfig &config);
  void _applyConnectionHeader(const HttpRequest &request,
                              HttpResponse &response);
  void _sendError(int errorCode, HttpResponse &response,
                  const ServerConfig &config, const HttpRequest &request,
                  const LocationConfig *location = NULL);
};
