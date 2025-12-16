#ifndef UTILSRESPONSE_HPP
#define UTILSRESPONSE_HPP

#include <string>

std::string getHttpDate();

std::string sizeToString(size_t value);

std::string getHttpStatusMessage(int code);

#endif