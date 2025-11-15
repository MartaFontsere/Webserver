#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include "../../includes/config_parser/DirectiveParser.hpp"
#include "../../includes/config_parser/BlockParser.hpp"

std::string trimLine(const std::string &line)
{
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

bool isEmptyOrComment(const std::string &trimmedLine)
{
    std::string temp = trimLine(trimmedLine);
    return temp.empty() || temp[0] == '#';
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t pos = str.find(delimiter, start);

    while (pos != std::string::npos)
    {
        tokens.push_back(str.substr(start, pos - start));
        start = pos + 1;
        pos = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));
    return tokens;
}
