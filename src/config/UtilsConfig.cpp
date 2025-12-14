#include "../../includes/config/UtilsConfig.hpp"

int stringToInt(const std::string &value)
{
    std::stringstream ss(value);
    int number = 0;
    ss >> number;
    return number;
}