#include "../../includes/cgi/CGIEnnviroment.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

void CGIEnvironment::prepare(const Request &req, const std::string &scriptPath,
                const std::string &scriptName, const std::string &serverName,
                int serverPort)
{
    std::map<std::string, std::string> httpEnvVars = convertHeadersToEnv(req.getHeaders());
    std::map<std::string, std::string>::const_iterator it;
    for (it = httpEnvVars.begin(); it != httpEnvVars.end(); ++it)
    {
        _envVars[it->first] = it->second;
    }

    _envVars["GATEWAY_INTERFACE"] = "CGI/1.1";
    _envVars["SERVER_SOFTWARE"] = "webserv/1.0";
    _envVars["SERVER_PROTOCOL"] = "HTTP/1.1";
    _envVars["SERVER_NAME"] = serverName;
    _envVars["SERVER_PORT"] = intToString(serverPort);
    _envVars["REQUEST_METHOD"] = req.getMethod();
    _envVars["QUERY_STRING"] = extractQueryString(req.getURI());
    _envVars["SCRIPT_NAME"] = scriptName;
    _envVars["SCRIPT_FILENAME"] = scriptPath;
    _envVars["CONTENT_TYPE"] = req.getHeader("Content-Type");
    _envVars["CONTENT_LENGTH"] = intToString(req.getBody().size());
}