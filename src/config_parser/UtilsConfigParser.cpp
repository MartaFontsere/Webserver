#include <iostream>
#include <fstream>
#include <sstream>
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

std::vector<std::string> tokenize(const std::string &line, int numLine)
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
    {
        std::stringstream message;
        message << "Warning: Unclosed quote in line: " << numLine;
        throw std::runtime_error(message.str());
    }
    return tokens;
}

BlockParser readConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        throw std::runtime_error("❌ File can't be open");

    BlockParser root;
    std::string line;
    std::string accumulated;
    int lineNumber = 0;
    int directiveStartLine = 0;

    while (std::getline(file, line))
    {
        lineNumber++;

        std::string trimmed = trimLine(line);
        if (isEmptyOrComment(trimmed))
            continue;
        
        size_t commentPos = trimmed.find('#');
        if (commentPos != std::string::npos)
        {
            trimmed = trimmed.substr(0, commentPos);
            trimmed = trimLine(trimmed);
        }

        if (trimmed.empty())
            continue;

        if (accumulated.empty())
            directiveStartLine = lineNumber;

        if (!accumulated.empty())
            accumulated += " ";
        accumulated += trimmed;

        if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string blockLine = accumulated;
            std::string blockName = blockLine.substr(0, blockLine.size() - 1);
            blockName = trimLine(blockName);
            BlockParser temp;
            BlockParser nest = temp.parseBlock(file, blockName, lineNumber);
            root.addNest(nest);
            accumulated.clear();
        }
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            DirectiveParser parser;
            accumulated = accumulated.substr(0, accumulated.size() - 1);
            std::vector<std::string> tokens = tokenize(accumulated, lineNumber);
            parser.parseDirective(tokens, directiveStartLine);
            const std::vector<DirectiveToken> &dirs = parser.getDirectives();
            for (size_t i = 0; i < dirs.size(); ++i)
                root.addDirective(dirs[i]);
            accumulated.clear();
        }
    }
    if (!accumulated.empty())
    {
        std::stringstream message;
        message << "⚠️ Error: Unterminated directive at EOF \n"
                << "  Started at line: " << directiveStartLine << "\n  Content: " << accumulated;
        throw std::runtime_error(message.str());
    }
    return root;
}
