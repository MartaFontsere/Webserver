#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
#include "../../includes/config_parser/ValidationConfigFile.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"

bool isEmptyBraceOrSemicolonLine(std::string line, int *lineCont, const std::string &filePath)
{
    std::string temp;
    temp = trimLine(line);
    if (temp[0] == '{')
    {
        std::cerr << "Error: no name before '{' in line" << "(" << *lineCont << ") in file: " << filePath << std::endl;
        return true;
    }
    else if (temp[0] == ';')
    {
        std::cerr << "Error: no name before ';' in line" << "(" << *lineCont << ") in file: " << filePath << std::endl;
        return true;
    }
    return false;
}
int contOpenKeys(std::string line, int *lineCont, int *contOpenKey)
{
    int firstOpenKey = 0;
    std::string temp = trimLine(line);

    if (temp[temp.size() - 1] == '{')
    {
        (*contOpenKey)++;
        if (*contOpenKey == 1)
            firstOpenKey = *lineCont;
    }
    return firstOpenKey;
}
int contCloseKeys(std::string line, int *lineCont, int *contCloseKey)
{
    int lastCloseKey = 0;
    std::string temp = trimLine(line);
    if (temp[temp.size() - 1] == '}')
    {
        (*contCloseKey)++;
        lastCloseKey = *lineCont;
    }
    return lastCloseKey;
}
void processConfigLine(const std::string &line, int *lineCont, int *contOpenKey,
                       int *contCloseKey, int *firstOpenKey, int *lastCloseKey)
{
    int openLine = contOpenKeys(line, lineCont, contOpenKey);
    int closeLine = contCloseKeys(line, lineCont, contCloseKey);
    if (openLine > 0 && *firstOpenKey == 0)
        *firstOpenKey = openLine;
    if (closeLine > 0)
        *lastCloseKey = closeLine;
}

bool resultProcesConfigLine(int contOpenKey, int contCloseKey, int firstOpenKey, int lastCloseKey, const std::string &filePath)
{
    if (contOpenKey > contCloseKey)
    {
        std::cerr << "Error: Expected '}' in line" << "(" << firstOpenKey << ") in file: " << filePath << std::endl;
        return true;
    }
    else if (contOpenKey < contCloseKey)
    {
        std::cerr << "Error: extraneous closing bace '}' in line" << "(" << lastCloseKey << ") in file: " << filePath << std::endl;
        return true;
    }
    return false;
}

bool incorrectLineTermination(const std::string &line, int *lineCont, const std::string &filePath)
{
    std::string temp = trimLine(line);
    if ((temp[temp.size() - 1] != '{') && (temp[temp.size() - 1] != ';') && (temp[temp.size() - 1] != '}'))
    {
        std::cerr << "⚠️ Warning: Malformed line ending detected.\n"
                  << "Please check if a closing character ('{', '}' or ';') is missing in line: "
                  << "(" << *lineCont << ") in file: " << filePath << std::endl;
        return true;
    }

    return false;
}
bool firstNonAlNumChar(const std::string &line, int *lineCont, const std::string &filePath)
{
    std::string temp = trimLine(line);
    bool dir;
    size_t i = 0;
    while (i < line.size() - 1)
    {
        if (std::isalnum(static_cast<unsigned char>(line[i])) && (line[i + 1] == ' ' && line[i + 2] == '.' && line[i + 3] == '/'))
            dir = true;
        else if (!std::isalnum(static_cast<unsigned char>(line[i])) && (line[i] != ' ' && line[i] != '_') && !dir)
        {
            if (line[i] != '\0')
            {
                std::cerr << "Error: Character ('" << line[i] << "') Not Allowed in line " << "(" << *lineCont << ") in file: " << filePath << std::endl;
                return true;
            }
        }
        ++i;
    }
    return false;
}