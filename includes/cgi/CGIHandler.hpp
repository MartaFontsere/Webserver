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

class CGIHandler {
public:
  CGIHandler();
  ~CGIHandler();

  // Original synchronous handle (kept for backwards compatibility)
  HttpResponse handle(const HttpRequest &request,
                      const LocationConfig &location,
                      const std::string &serverName, int serverPort);

  // NEW: Async handle - forks CGI but doesn't wait, returns pipe FD and PID
  CGIAsyncResult handleAsync(const HttpRequest &request,
                             const LocationConfig &location,
                             const std::string &serverName, int serverPort);

  // NEW: Build response from completed CGI output
  HttpResponse buildResponseFromCGIOutput(const std::string &cgiOutput);

private:
  std::string resolveScriptPath(const HttpRequest &request,
                                const LocationConfig &location);
  std::string getScriptName(const HttpRequest &request);
};

#endif