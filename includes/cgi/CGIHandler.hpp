#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../config/LocationConfig.hpp"
#include "CGIDetector.hpp"
#include "CGIEnvironment.hpp"
#include "CGIExecutor.hpp"
#include "CGIOutputParser.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <string>

/**
 * @brief CGI orchestrator - coordinates detection, environment, and execution
 */
class CGIHandler {
public:
  CGIHandler();
  ~CGIHandler();

  /** @brief Synchronous CGI execution (blocks until complete) */
  HttpResponse handle(const HttpRequest &request,
                      const LocationConfig &location,
                      const std::string &serverName, int serverPort);

  /** @brief Async CGI execution - forks but doesn't wait */
  CGIAsyncResult handleAsync(const HttpRequest &request,
                             const LocationConfig &location,
                             const std::string &serverName, int serverPort);

  /** @brief Build HTTP response from completed CGI output */
  HttpResponse buildResponseFromCGIOutput(const std::string &cgiOutput);

private:
  std::string resolveScriptPath(const HttpRequest &request,
                                const LocationConfig &location);
  std::string getScriptName(const HttpRequest &request);
};

#endif