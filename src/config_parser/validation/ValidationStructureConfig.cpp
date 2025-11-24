#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cctype>
#include <sstream>
#include "../../../includes/config_parser/validation/ValidationStructureConfig.hpp"
#include "../../../includes/config_parser/parser/UtilsConfigParser.hpp"

void checkEmptyBraceOrSemicolon(const std::string &trimmedLine, int lineCont,
                                const std::string &filePath,
                                std::vector<std::string> &errors)
{
    if (trimmedLine[0] == '{')
    {
        std::stringstream message;
        message << "Error line " << lineCont
                << ": No name before '{' in " << filePath;
        errors.push_back(message.str());
    }
    else if (trimmedLine[0] == ';')
    {
        std::stringstream message;
        message << "Error line " << lineCont
                << ": No name before ';' in " << filePath;
        errors.push_back(message.str());
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
    case '#':
    case '~':
    case '^':
    case '\\':
    case '|':
    case '(':
    case ')':
    case '[':
    case ']':
    case '+':
    case '?':
        return true;

    default:
        return false;
    }
}

void checkInvalidCharacters(const std::string &trimmedLine, int lineCont, std::vector<std::string> &errors)
{
    for (size_t i = 0; i < trimmedLine.size(); ++i)
    {
        if (!isValidConfigChar(trimmedLine[i]))
        {
            std::stringstream message;
            message << "Error line " << lineCont
                    << ": Invalid character '" << trimmedLine[i] << "'";
            errors.push_back(message.str());
            return;
        }
    }
}

void checkBraceBalance(int contOpenKey, int contCloseKey, int firstOpenKey,
                       int lastCloseKey, const std::string &filePath, std::vector<std::string> &errors)
{
    (void)filePath; // No necesitamos filePath

    if (contOpenKey > contCloseKey)
    {
        std::stringstream message;
        message << "Error line " << firstOpenKey
                << ": Missing closing brace '}'";
        errors.push_back(message.str());
    }
    else if (contOpenKey < contCloseKey)
    {
        std::stringstream message;
        message << "Error line " << lastCloseKey
                << ": Unexpected closing brace '}'";
        errors.push_back(message.str());
    }
}

bool validateStructure(const std::string &filePath, std::vector<std::string> &errors)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
    {
        errors.push_back("Error: Cannot open file '" + filePath + "'");
        return false;
    }
    std::string line;
    int contOpenKey = 0;
    int contCloseKey = 0;
    int firstOpenKey = 0;
    int lastCloseKey = 0;
    int lineCont = 1;

    while (std::getline(file, line))
    {
        if (!isEmptyOrComment(line))
        {
            std::string trimmed = trimLine(line);
            size_t commentPos = trimmed.find('#');
            if (commentPos != std::string::npos)
            {
                trimmed = trimmed.substr(0, commentPos);
                trimmed = trimLine(trimmed);
            }
            if (!trimmed.empty())
            {
                checkEmptyBraceOrSemicolon(trimmed, lineCont, filePath, errors);
                checkInvalidCharacters(trimmed, lineCont, errors);
                processConfigLine(trimmed, lineCont, contOpenKey, contCloseKey,
                                  firstOpenKey, lastCloseKey);
            }
        }
        lineCont++;
    }
    checkBraceBalance(contOpenKey, contCloseKey, firstOpenKey, lastCloseKey,
                      filePath, errors);
    return errors.empty();
}
