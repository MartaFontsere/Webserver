#ifndef BLOCKPARSER_HPP
#define BLOCKPARSER_HPP

#include <string>
#include <vector>
#include "DirectiveParser.hpp"

class BlockParser
{

private:
    std::string name;
    int startLine;
    int endLine;
    std::vector<DirectiveToken> directives;
    std::vector<BlockParser> nestedBlocks;  

public:
    BlockParser();
    BlockParser(const std::string &blockName, int start = 0);
    BlockParser &operator=(const BlockParser &other);
    ~BlockParser();

    std::string getName() const;
    std::vector<DirectiveToken> getDirectives() const;
    std::vector<BlockParser> getNestedBlocks() const;
    int getStartLine() const;
    int getEndLine() const;

    void setName(const std::string &newName);
    void setEndLine(int line);

    void addDirective(const DirectiveToken &directive);
    void addNest(const BlockParser &nest);

    static BlockParser parseBlock(std::ifstream &file, const std::string &blockName, int &lineNumber);
    void printBlock(const BlockParser &block);
}; 

#endif
