#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
#include <sstream>
#include "../../includes/config_parser/ValidationStructureConfig.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"

void isEmptyBraceOrSemicolonLine(const std::string &trimmedLine, int &lineCont, const std::string &filePath)
{
    if (trimmedLine[0] == '{')
    {
        std::stringstream message1;
        message1 << "Error: no name before '{' in line" << "(" << lineCont << ") in file: " << filePath;
        throw std::runtime_error(message1.str());
    }
    else if (trimmedLine[0] == ';')
    {
        std::stringstream message2;
        message2 << "Error: no name before ';' in line" << "(" << lineCont << ") in file: " << filePath;
        throw std::runtime_error(message2.str());
    }
}
int contOpenKeys(const std::string &trimmedLine, int &lineCont, int &contOpenKey)
{
    int firstOpenKey = 0;
    if (trimmedLine[trimmedLine.size() - 1] == '{')
    {
        contOpenKey++;
        if (contOpenKey == 1)
            firstOpenKey = lineCont;
    }
    return firstOpenKey;
}
int contCloseKeys(const std::string &trimmedLine, int &lineCont, int &contCloseKey)
{
    int lastCloseKey = 0;
    if (trimmedLine[trimmedLine.size() - 1] == '}')
    {
        contCloseKey++;
        lastCloseKey = lineCont;
    }
    return lastCloseKey;
}
void processConfigLine(const std::string &trimmedLine, int &lineCont, int &contOpenKey,
                       int &contCloseKey, int &firstOpenKey, int &lastCloseKey)
{
    int openLine = contOpenKeys(trimmedLine, lineCont, contOpenKey);
    int closeLine = contCloseKeys(trimmedLine, lineCont, contCloseKey);
    if (openLine > 0 && firstOpenKey == 0)
        firstOpenKey = openLine;
    if (closeLine > 0)
        lastCloseKey = closeLine;
}

void resultProcesConfigLine(int contOpenKey, int contCloseKey, int firstOpenKey, int lastCloseKey, const std::string &filePath)
{
    if (contOpenKey > contCloseKey)
    {
        std::stringstream message3;
        message3 << "Error: Expected '}' in line" << "(" << firstOpenKey << ") in file: " << filePath;
        throw std::runtime_error(message3.str());
    }
    else if (contOpenKey < contCloseKey)
    {
        std::stringstream message4;
        message4 << "Error: extraneous closing bace '}' in line" << "(" << lastCloseKey << ") in file: " << filePath;
        throw std::runtime_error(message4.str());
    }
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

void firstNonAlNumChar(const std::string &trimmedLine, int &lineCont, const std::string &filePath)
{
    for (size_t i = 0; i < trimmedLine.size(); ++i)
    {
        if (!isValidConfigChar(trimmedLine[i]))
        {
            std::stringstream message5;
            message5 << "Error: Character ('" << trimmedLine[i] << "') not allowed "
                     << "in line (" << lineCont << ") "
                     << "in file: " << filePath;
            throw std::runtime_error(message5.str());
        }
    }
}
