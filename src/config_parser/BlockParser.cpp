#include "../../includes/config_parser/BlockParser.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include <string>
#include <iostream>
#include <fstream>

// Constructor por defecto
BlockParser::BlockParser()
{
    // Inicialización si es necesario
}

BlockParser::BlockParser(const std::string &blockName) : name(blockName) {};

// Operador de asignación
BlockParser &BlockParser::operator=(const BlockParser &other)
{
    if (this != &other)
    {
        name = other.name;
        directives = other.directives;
        nestedBlocks = other.nestedBlocks;
    }
    return *this;
}

// Destructor
BlockParser::~BlockParser()
{
    // No hay recursos que liberar
}

// Getters
std::string BlockParser::getName() const
{
    return name;
}

std::vector<DirectiveToken> BlockParser::getDirectives() const
{
    return directives;
}

std::vector<BlockParser> BlockParser::getNestedBlocks() const
{
    return nestedBlocks;
}

// Metodos propios
void BlockParser::setName(const std::string &newName)
{
    name = newName;
}

void BlockParser::addDirective(const DirectiveToken &directive)
{
    directives.push_back(directive);
}

void BlockParser::addNest(const BlockParser &nest)
{
    nestedBlocks.push_back(nest);
}

/**
 * @brief
 *
 * @param file fsdkfd lkdajflsk  slkfjslkfg
 * @param blockName fkjsdlkf
 * @return BlockParser
 */
BlockParser BlockParser::parseBlock(std::ifstream &file, const std::string &blockName)
{
    DirectiveParser parser;
    BlockParser block(blockName);

    std::string line;
    while (std::getline(file, line))
    {
        std::string trimmed = trimLine(line);
        if (isEmptyOrComment(trimmed))
            continue;

        if (trimmed[trimmed.size() - 1] == '{')
        {
            // Detectar subbloque
            std::string childName = trimmed.substr(0, trimmed.size() - 1);
            childName = trimLine(childName);
            BlockParser child = parseBlock(file, childName);
            block.addNest(child);
        }
        else if (trimmed == "}")
        {
            const std::vector<DirectiveToken> &parsed = parser.getDirectives();
            for (size_t i = 0; i < parsed.size(); ++i)
                block.addDirective(parsed[i]);
            return block;
        }
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            // Directiva simple
            trimmed = trimmed.substr(0, trimmed.size() - 1);
            std::vector<std::string> tokens = split(trimmed, ' ');
            if (!parser.parseDirective(tokens))
                std::cerr << "⚠️ Error parseando directiva: " << trimmed << std::endl;
        }
        else
        {
            std::cerr << "❓ Línea desconocida dentro del bloque " << blockName << ": " << trimmed << std::endl;
        }
    }

    // En caso de EOF sin cerrar el bloque
    std::cerr << "❌ Error: bloque '" << blockName << "' no cerrado correctamente" << std::endl;
    return block;
}
void BlockParser::printBlock(const BlockParser &block)
{
    // 1. IMPRIMIR CABECERA DEL BLOQUE
    // Se añade un salto de línea inicial para separar los bloques
    std::cout << "\n=== BLOCK ===" << std::endl;
    std::cout << "NAME: " << block.name << std::endl;
    std::cout << "---------------------------------" << std::endl;

    // 2. ITERAR E IMPRIMIR DIRECTIVAS
    const std::vector<DirectiveToken> &directives = block.getDirectives();
    for (size_t i = 0; i < directives.size(); ++i)
    {
        // Cabecera de la Directiva
        std::cout << "  Directive [" << i << "]:" << std::endl; // Espacio fijo para claridad
        std::cout << "    NAME: " << directives[i].name << std::endl;

        // Valores de la Directiva
        const std::vector<std::string> &values = directives[i].values;
        for (size_t j = 0; j < values.size(); ++j)
        {
            std::cout << "    VALUE [" << j << "]: " << values[j] << std::endl;
        }

        // Separador de Directivas
        std::cout << "  ------------------" << std::endl;
    }

    // 3. ITERAR SOBRE BLOQUES ANIDADOS (RECURSIÓN)
    const std::vector<BlockParser> &nestedBlocks = block.getNestedBlocks();
    for (size_t i = 0; i < nestedBlocks.size(); ++i)
    {
        // Llamada Recursiva: Solo se pasa el bloque.
        printBlock(nestedBlocks[i]);
    }

    // Separador final del bloque
    std::cout << "--- END BLOCK (" << block.name << ") ---" << std::endl;
    std::cout << "////////////////////////////////////" << std::endl;
}

/*void BlockParser::printBlock(const BlockParser &block, int indent)
{
    std::string pad(indent, ' ');
    std::cout << pad << "Block: " << block.name << std::endl;

    for (size_t i = 0; i < block.getDirectives().size(); ++i)
    {
        std::cout << pad << "  Directive: " << block.getDirectives()[i].name << " = ";
        for (size_t j = 0; j < block.getDirectives()[i].values.size(); ++j)
        {
            if (j > 0)
                std::cout << ", ";
            std::cout << block.getDirectives()[i].values[j];
        }
        std::cout << std::endl;
    }

    for (size_t i = 0; i < block.getNestedBlocks().size(); ++i)
        printBlock(block.getNestedBlocks()[i], indent + 2);
}*/