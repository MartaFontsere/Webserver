#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/ValidationStructureConfig.hpp"
#include <fstream>
#include <iostream>

BlockParser readConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        throw std::runtime_error("❌ File can't be open");
    BlockParser root;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line))
    {
        lineNumber++;
        std::string trimmed = trimLine(line);
        if (isEmptyOrComment(trimmed))
            continue;
        if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string blockName = trimmed.substr(0, trimmed.size() - 1);
            blockName = trimLine(blockName);
            BlockParser temp;
            BlockParser nest = temp.parseBlock(file, blockName, lineNumber);
            root.addNest(nest);
        }
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            DirectiveParser parser;
            trimmed = trimmed.substr(0, trimmed.size() - 1);
            std::vector<std::string> tokens = tokenize(trimmed);
            parser.parseDirective(tokens, lineNumber);
            const std::vector<DirectiveToken> &dirs = parser.getDirectives();
            for (size_t i = 0; i < dirs.size(); ++i)
                root.addDirective(dirs[i]);
        }
        else
            std::cout << "❓ Unknow out of block at line: "<< lineNumber << " => "<< trimmed << std::endl;
    }
    return root;
}

bool validationStructureConfigFile(const std::string &filePath)
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
            if(!trimmed.empty())
            {
                if (isEmptyBraceOrSemicolonLine(trimmed, lineCont, filePath))
                    return false;
                else if (firstNonAlNumChar(trimmed, lineCont, filePath))
                    return false;
                processConfigLine(trimmed, lineCont, contOpenKey, contCloseKey, firstOpenKey,
                            lastCloseKey);
            }

        }
        lineCont++;
    }
    if (resultProcesConfigLine(contOpenKey, contCloseKey, firstOpenKey, lastCloseKey, filePath))
        return false;
    return true;
}