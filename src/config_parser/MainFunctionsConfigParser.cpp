#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/ValidationConfigFile.hpp"
#include <fstream>
#include <iostream>

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

bool validationConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        throw std::runtime_error("❌ No se pudo abrir el archivo");
    std::string line;
    int contOpenKey = 0;
    int contCloseKey = 0;
    int firstOpenKey = 0;
    int lastCloseKey = 0;
    std::vector<std::string> lines;
    int lineCont = 1;
    while (std::getline(file, line))
    {
        if (isEmptyBraceOrSemicolonLine(line, &lineCont, filePath))
            return false;
        if (incorrectLineTermination(line, &lineCont, filePath))
            return false;
        if (firstNonAlNumChar(line, &lineCont, filePath))
            return false;

        processConfigLine(line, &lineCont, &contOpenKey, &contCloseKey, &firstOpenKey,
                          &lastCloseKey);
        ////ZONA DE CURRELE///
        // repeticiones de name o value???
        // TODO: MEJORAR EL TRIM PARA ESPACIOS POSTERIORES

        //////////////////////
        lines.push_back(line);
        lineCont++;
    }
    if (resultProcesConfigLine(contOpenKey, contCloseKey, firstOpenKey, lastCloseKey, filePath))
        return false;
    // size_t i = -1;
    // while (++i < lines.size())
    //     std::cout << "Line[" << i << "]:" << lines[i] << std::endl;
    return true;
}