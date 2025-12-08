#include "../../includes/cgi/CGIHandler.hpp"

CGIHandler::CGIHandler()
{
}

CGIHandler::~CGIHandler()
{
}

Response CGIHandler::handle(const Request &req, const LocationConfig &location)
{
    bool isCGI = CGIDetector::isCGIRequest(req.getURI(), location.cgiExts);

    if (!isCGI)
        return Response(404, "Not Found");

    std::string scriptPath = CGIDetector::resolveScriptPath(req.getURI(), location.root);

    std::string executable = CGIDetector::getCGIExecutable(scriptPath, location.cgiPaths, location.cgiExts);

    if (executable.empty())
        return Response(404, "Not Found");

    std::string scriptName = CGIDetector::removeQueryString(req.getURI());
    CGIEnvironment env;
    env.prepare(req, scriptPath, scriptName, location.serverName, location.serverPort);

    char **envp = env.toEnvArray();

    try
    {
        CGIExecutor executor;
        std::string output = executor.execute(executable, scriptPath, envp, req.getBody());

        CGIOutputParser parser;
        parser.parse(output);

        Response response(parser.getStatusCode(), parser.getBody());
        env.freeEnvArray(envp);
        return response;
    }
    catch (std::exception &e)
    {
        env.freeEnvArray(envp);
        return Response(500, "Internal Server Error");
    }
}