#ifndef BLOCKPARSER_HPP
#define BLOCKPARSER_HPP

#include <string>
#include <vector>
#include "DirectiveParser.hpp" // Para usar DirectiveToken

class BlockParser
{

private:
    std::string name;                       // Nombre del bloque (p. ej. "server", "location")
    std::vector<DirectiveToken> directives; // Directivas dentro del bloque
    std::vector<BlockParser> nestedBlocks;  // Subbloques

public:
    BlockParser();
    BlockParser(const std::string &blockName);
    BlockParser &operator=(const BlockParser &other);
    ~BlockParser();

    std::string getName() const;
    std::vector<DirectiveToken> getDirectives() const;
    std::vector<BlockParser> getNestedBlocks() const;

    void setName(const std::string &newName);

    void addDirective(const DirectiveToken &directive);
    void addNest(const BlockParser &nest);

    BlockParser parseBlock(std::ifstream &file, const std::string &blockName);
    void printBlock(const BlockParser &block);
}; /*, int indent = 0*/

#endif
