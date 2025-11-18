#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/ValidationStructureConfig.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

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

void validationStructureConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        throw std::runtime_error("❌ No se pudo abrir el archivo");
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
            if (!trimmed.empty())
            {
                isEmptyBraceOrSemicolonLine(trimmed, lineCont, filePath);
                firstNonAlNumChar(trimmed, lineCont, filePath);
                processConfigLine(trimmed, lineCont, contOpenKey, contCloseKey, firstOpenKey,
                                  lastCloseKey);
            }
        }
        lineCont++;
    }
    resultProcesConfigLine(contOpenKey, contCloseKey, firstOpenKey, lastCloseKey, filePath);
}