#ifndef CGI_UTILS_HPP
#define CGI_UTILS_HPP

#include <string>
#include <map>
#include <sstream>
#include <cctype>

std::string intToString(int value);

std::string extractQueryString(const std::string &uri);

std::map<std::string, std::string> convertHeadersToEnv(
    const std::map<std::string, std::string> &headers);

std::string headerToEnvName(const std::string &headerName);

std::string toUpperCase(const std::string &str);

#endif