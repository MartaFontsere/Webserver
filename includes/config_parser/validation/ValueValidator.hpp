#ifndef VALUEVALIDATOR_HPP
#define VALUEVALIDATOR_HPP

#include <string>

/** @brief Value type validators for directive arguments */
bool isValidNumber(const std::string &value);
bool isValidPort(const std::string &value);
bool isValidPath(const std::string &value);
bool isValidHost(const std::string &value);
bool isValidIP(const std::string &value);
bool isValidHttpCode(const std::string &value);
bool isValidBool(const std::string &value);
bool isValidPattern(const std::string &value);

#endif