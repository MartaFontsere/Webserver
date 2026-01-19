#ifndef CGI_UTILS_HPP
#define CGI_UTILS_HPP

#include <cctype>
#include <map>
#include <sstream>
#include <string>

/** @brief Convert integer to string (C++98 compatible) */
std::string intToString(int value);

/** @brief Extract query string from URI (after '?') */
std::string extractQueryString(const std::string &uri);

/** @brief Convert HTTP headers to CGI environment format */
std::map<std::string, std::string>
convertHeadersToEnv(const std::map<std::string, std::string> &headers);

/** @brief Convert header name to CGI env format (HTTP_CONTENT_TYPE) */
std::string headerToEnvName(const std::string &headerName);

/** @brief Convert string to uppercase */
std::string toUpperCase(const std::string &str);

/** @brief Copy std::string to C-string buffer */
void stringToCString(const std::string &source, char *dest);

#endif