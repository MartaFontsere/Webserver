#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "CGIDetector.hpp"
#include "CGIEnvironment.hpp"
#include "CGIExecutor.hpp"
#include "CGIOutputParser.hpp"
#include <string>

class CGIHandler {
public:
  CGIHandler();
  ~CGIHandler();

  HttpResponse handle(const HttpRequest &request,
                      const LocationConfig &location,
                      const std::string &serverName, int serverPort);

private:
  std::string resolveScriptPath(const HttpRequest &request,
                                const LocationConfig &location);
  std::string getScriptName(const HttpRequest &request);
};

#endif