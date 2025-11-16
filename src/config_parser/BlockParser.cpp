#include "../../includes/config_parser/BlockParser.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include <string>
#include <iostream>
#include <fstream>

// Construc
BlockParser::BlockParser() : startLine(0), endLine(0)
{
}

BlockParser::BlockParser(const std::string &blockName, int start) : name(blockName), startLine(start), endLine(0) {};

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

BlockParser::~BlockParser()
{

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

int BlockParser::getStartLine() const
{
    return startLine;
}

int BlockParser::getEndLine() const
{
    return endLine;
}

void BlockParser::setName(const std::string &newName)
{
    name = newName;
}

void BlockParser::setEndLine(int line)
{
    endLine = line;
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
BlockParser BlockParser::parseBlock(std::ifstream &file, const std::string &blockName, int &lineNumber)
{
    DirectiveParser parser;
    BlockParser block(blockName, lineNumber);

    std::string line;
    while (std::getline(file, line))
    {
        lineNumber++;
        std::string trimmed = trimLine(line);
        if (isEmptyOrComment(trimmed))
            continue;

        if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string childName = trimmed.substr(0, trimmed.size() - 1);
            childName = trimLine(childName);
            BlockParser child = parseBlock(file, childName, lineNumber);
            block.addNest(child);
        }
        else if (trimmed == "}")
        {
            block.setEndLine(lineNumber);
            const std::vector<DirectiveToken> &parsed = parser.getDirectives();
            for (size_t i = 0; i < parsed.size(); ++i)
                block.addDirective(parsed[i]);
            return block;
        }
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            trimmed = trimmed.substr(0, trimmed.size() - 1);
            std::vector<std::string> tokens = tokenize(trimmed);
            if (!parser.parseDirective(tokens, lineNumber))
                std::cerr << "⚠️ Error parsing directive: " << trimmed << " at line: " << lineNumber << std::endl;
        }
        else
        {
            std::cerr << "❓ Unknown line at block: " << blockName << ": in line:" << lineNumber << " =>"<< trimmed << std::endl;
        }
    }
    std::cerr << "❌ Error: block '" << blockName << "' not closed properly"
                    << " (started at line: " << block.getStartLine() << ")" << std::endl;
    return block;
}
void BlockParser::printBlock(const BlockParser &block)
{
    std::cout << "\n=== BLOCK ===" << std::endl;
    std::cout << "NAME: " << block.name << std::endl;
    std::cout << "LINES: " << block.startLine << " - " << block.endLine << std::endl;
    std::cout << "---------------------------------" << std::endl;

    const std::vector<DirectiveToken> &directives = block.getDirectives();
    for (size_t i = 0; i < directives.size(); ++i)
    {
        std::cout << "  Directive [" << i << "] (line " << directives[i].lineNumber << "): "<< std::endl;
        std::cout << "    NAME: " << directives[i].name << std::endl;

        const std::vector<std::string> &values = directives[i].values;
        for (size_t j = 0; j < values.size(); ++j)
            std::cout << "    VALUE [" << j << "]: " << values[j] << std::endl;
        std::cout << "  ------------------" << std::endl;
    }
    const std::vector<BlockParser> &nestedBlocks = block.getNestedBlocks();
    for (size_t i = 0; i < nestedBlocks.size(); ++i)
        printBlock(nestedBlocks[i]);
    std::cout << "--- END BLOCK (" << block.name << ") ---" << std::endl;
    std::cout << "////////////////////////////////////" << std::endl;
}