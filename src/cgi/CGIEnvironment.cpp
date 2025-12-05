#include "../../includes/cgi/CGIEnvironment.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

CGIEnvironment::CGIEnvironment()
{
}

CGIEnvironment::~CGIEnvironment()
{
}

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

char **CGIEnvironment::toEnvArray() const
{
    size_t count =  _envVars.size();
    size_t index = 0;

    char **env = new char*[count + 1];

    std::map<std::string, std::string>::const_iterator it;
    for (it = _envVars.begin(); it != _envVars.end(); ++it)
    {
        std::string envLine = it->first + "=" + it->second;
        int envLineLength = envLine.size();
        env[index] = new char[envLineLength + 1];
        stringToCString(envLine, env[index]);
        index++;
    }
    env[count] = NULL;
    return env;  

}

void CGIEnvironment::freeEnvArray(char **env) const
{
    for (int i = 0; env[i]; i++)
    {
        delete[] env[i];
    }
    delete[] env;
}

std::string CGIEnvironment::getVar(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it;
    it = _envVars.find(key);
    if (it != _envVars.end())
        return it->second;
    else
        return "";
}

void CGIEnvironment::printAll() const
{

    std::cout << "=== CGI ENVIRONMENT VARIABLES ===" << std::endl;
    std::map<std::string, std::string>::const_iterator it;
    for (it = _envVars.begin(); it != _envVars.end(); ++it)
    {
        std::cout << it->first << " = " << it->second << std::endl;
    }
}
