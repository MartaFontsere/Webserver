#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include "hardcoded/Request.hpp"
#include "response/HttpResponse.hpp"
#include "response/UtilsResponse.hpp"
#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "CGIDetector.hpp"
#include "CGIEnvironment.hpp"
#include "CGIExecutor.hpp"
#include "CGIOutputParser.hpp"

class CGIHandler
{
public:
    CGIHandler();
    ~CGIHandler();

    HttpResponse handle(const Request &req, const LocationConfig &location, const ServerConfig &server);

private:
    std::string resolveScriptPath(const Request &req, const LocationConfig &location);
    std::string getScriptName(const Request &req);
};

#endif