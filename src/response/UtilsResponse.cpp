#include "../../includes/response/UtilsResponse.hpp"

#include <string>
#include <ctime>
#include <sstream>

std::string getHttpDate()
{
    time_t currentTime;
    time(&currentTime);

    struct tm *timeInfo = gmtime(&currentTime);
    char buffer[80];
    strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);

    std::string result = buffer;
    return result;
}

std::string sizeToString(size_t value)
{
    std::ostringstream oss;
    oss << value;

    return oss.str();
}