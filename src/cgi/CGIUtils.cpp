#include "../../includes/cgi/CGIUtils.hpp"

std::string intToString(int value)
{
    std::stringstream stringResult;
    stringResult << value;
    return stringResult.str();
}

std::string extractQueryString(const std::string &uri)
{
    size_t questPos = uri.find('?');
    if (questPos != std::string::npos)
    {
        std::string result = uri.substr(questPos + 1);
        return result;
    }
    return "";
}

std::map<std::string, std::string> convertHeadersToEnv(
    const std::map<std::string, std::string> &headers)
{
    std::map<std::string, std::string> group;

    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        std::string envName = headerToEnvName(it->first);
        group[envName] = it->second;
    }

    return group;
}

std::string headerToEnvName(const std::string &headerName)
{
    std::string result;
    for (int i = 0; i < (int)headerName.length(); i++)
    {
        if (headerName[i] == '-')
            result += '_';
        else
            result += headerName[i];
    }
    return "HTTP_" + toUpperCase(result);
}

std::string toUpperCase(const std::string &str)
{
    std::string result;
    for (int i = 0; i < (int)str.length(); i++)
        result += std::toupper(str[i]);
    return result;
}

void stringToCString(const std::string &source, char *dest)
{
    for (size_t i = 0; i < source.size(); ++i)
    {
        dest[i] = source[i];
    }
    dest[source.size()] = '\0';
}