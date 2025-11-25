#include "../../includes/cgi/CGIDetector.hpp"
#include <vector>
#include <string>

std::string CGIDetector::getExtension(const std::string &path)
{
    size_t lastDot = path.find_last_of('.');
    size_t questPos = path.find('?');
    if (lastDot == std::string::npos || lastDot == path.size() - 1)
        return "";
    if (questPos != std::string::npos && questPos > lastDot)
        return path.substr(lastDot, (questPos - lastDot));
    return path.substr(lastDot);
}

std::string CGIDetector::removeQueryString(const std::string &uri)
{
    size_t quest = uri.find('?');

    if (quest == std::string::npos)
        return uri;
    return uri.substr(0, quest);
}

bool CGIDetector::isCGIRequest(const std::string &uri, const std::vector<std::string> &cgiExsts)
{
    std::string ext = getExtension(uri);

    for (size_t i = 0; i < cgiExsts.size(); ++i)
    {
        if (cgiExsts[i] == ext)
            return true;
    }
    return false;
}

std::string CGIDetector::getCGIExecutable(const std::string &scriptPath, const std::vector<std::string> &cgiPaths, 
                                    const std::vector<std::string> &cgiExsts)
{
    std::string ext = getExtension(scriptPath);

    if (ext.empty())
        return "";
    for (size_t i = 0; i < cgiExsts.size(); ++i)
    {
        if (cgiExsts[i] == ext)
            return cgiPaths[0];
    }
    return "";
}

std::string CGIDetector::resolveScriptPath(const std::string &uri, const std::string &root)
{
    std::string headUri = removeQueryString(uri);
    char firstUri = headUri[0];
    char lastRoot = root[root.size() - 1];

    if (headUri.empty())
        return root;
    if ((lastRoot != '/' && firstUri == '/') || (lastRoot == '/' && firstUri != '/'))
        return root + headUri;
    if (lastRoot != '/' && firstUri != '/')
        return root + '/' + headUri;
    if (lastRoot == '/' && firstUri == '/')
    {
        std::string root2 = root.substr(0, root.size() - 1);
        return root2 + headUri;
    }
    return ""; //pendiente comprobar en compilacion si es necesario o no.
}