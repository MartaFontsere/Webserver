#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include "hardcoded/Request.hpp"
#include "hardcoded/Response.hpp"
#include "hardcoded/LocationConfig.hpp"
#include "CGIDetector.hpp"
#include "CGIEnvironment.hpp"
#include "CGIExecutor.hpp"
#include "CGIOutputParser.hpp"

class CGIHandler
{
public:
    CGIHandler();
    ~CGIHandler();

    Response handle(const Request &req, const LocationConfig &location);

private:
    std::string resolveScriptPath(const Request &req, const LocationConfig &location);
    std::string getScriptName(const Request &req);
};

#endif