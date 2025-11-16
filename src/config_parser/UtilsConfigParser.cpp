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

std::vector<std::string> tokenize(const std::string &line)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < line.size(); ++i)
    {
        char temp = line[i];
        if (!inQuotes && (temp == '"' || temp == '\''))
        {
            inQuotes = true;
            quoteChar = temp;
            continue;
        }
        if (inQuotes && temp == quoteChar)
        {
            inQuotes = false;
            quoteChar = '\0';
            continue;
        }
        if (!inQuotes && (temp == ' ' || temp == '\t'))
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current += temp;
    }
    if (!current.empty())
        tokens.push_back(current);
    if (inQuotes)
        std::cerr << "Warning: Unclosed quote in line " << line << std::endl;
    return tokens;
}
