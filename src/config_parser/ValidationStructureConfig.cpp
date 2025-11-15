#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
#include "../../includes/config_parser/ValidationStructureConfig.hpp"
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
    if (temp.empty())
        return false;
    if ((temp[temp.size() - 1] != '{') && (temp[temp.size() - 1] != ';') && (temp[temp.size() - 1] != '}'))
    {
        std::cerr << "❌ Error: Line does not end with a valid closing character ('{', '}' or ';').\n"
                  << "Check syntax in line (" << *lineCont << ") of file: " << filePath << std::endl;
        return true;
    }

    return false;
}

static bool isValidConfigChar(char character)
{
    unsigned char temp = static_cast<unsigned char>(character);

    if (std::isalnum(temp))
        return true;

    switch (character)
    {
    case '/':
    case '.':
    case '_':
    case '-':
    case ':':
    case '*':
    case ',':
    case '=':
    case '@':
    case '$':
    case '"':
    case '\'':
    case ' ':
    case '\t':
    case ';':
    case '{':
    case '}':
        return true;

    default:
        return false;
    }
}

bool firstNonAlNumChar(const std::string &line, int *lineCont, const std::string &filePath)
{
    std::string temp = trimLine(line);

    for (size_t i = 0; i < temp.size(); ++i)
    {
        if (!isValidConfigChar(temp[i]))
        {
            std::cerr << "Error: Character ('" << temp[i] << "') not allowed "
                      << "in line (" << *lineCont << ") "
                      << "in file: " << filePath << std::endl;
            return true;
        }
    }

    return false;
}
