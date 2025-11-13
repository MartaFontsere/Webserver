#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include "DirectiveParser.hpp"
#include "BlockParser.hpp"

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
    return trimmedLine.empty() || trimmedLine[0] == '#';
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

BlockParser readConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        throw std::runtime_error("❌ No se pudo abrir el archivo");
    BlockParser root;
    std::string line;
    while (std::getline(file, line))
    {
        std::string trimmed = trimLine(line);
        if (isEmptyOrComment(trimmed))
            continue;
        if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string blockName = trimmed.substr(0, trimmed.size() - 1);
            blockName = trimLine(blockName);
            BlockParser temp;
            BlockParser nest = temp.parseBlock(file, blockName);
            root.addNest(nest);
        }
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            DirectiveParser parser;
            trimmed = trimmed.substr(0, trimmed.size() - 1);
            std::vector<std::string> tokens = split(trimmed, ' ');
            parser.parseDirective(tokens);
            const std::vector<DirectiveToken> &dirs = parser.getDirectives();
            for (size_t i = 0; i < dirs.size(); ++i)
                root.addDirective(dirs[i]);
        }
        else
            std::cout << "❓ DESCONOCIDO FUERA DE BLOQUE: " << trimmed << std::endl;
    }
    return root;
}
