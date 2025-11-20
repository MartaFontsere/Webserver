#include "../../includes/config_parser/BlockParser.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/DirectiveMetadata.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

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

static bool isDirectiveStart(const std::string &line)
{
    if (line.empty())
        return false;

    size_t spacePos = line.find(' ');
    std::string firstWord;

    if (spacePos == std::string::npos)
        firstWord = line;
    else
        firstWord = line.substr(0, spacePos);

    return (DirectiveMetadata::getRule(firstWord) != NULL);
}

BlockParser BlockParser::parseBlock(std::ifstream &file, const std::string &blockName, int &lineNumber)
{
    DirectiveParser parser;
    BlockParser block(blockName, lineNumber);

    std::string line;
    std::string accumulated;
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

        if (!accumulated.empty() && isDirectiveStart(trimmed))
        {
            std::stringstream message;
            message << "❌ Unterminated directive at line " << directiveStartLine << "\n  Content: '" << accumulated << "'"
                    << "\n  Missing ';' before line " << lineNumber << ": '" << trimmed << "'";
            throw std::runtime_error(message.str());
        }

        if (accumulated.empty())
            directiveStartLine = lineNumber;

        if (!accumulated.empty())
            accumulated += " ";

        accumulated += trimmed;
        if (trimmed == "}")
        {

            if (accumulated != "}")
            {
                std::stringstream message2;
                message2 << "⚠️ Error: Unterminated directive before '}' at line: "
                        << lineNumber << "\n  Content: " << accumulated;
                throw std::runtime_error(message2.str());
            }
            block.setEndLine(lineNumber);
            const std::vector<DirectiveToken> &parsed = parser.getDirectives();
            for (size_t i = 0; i < parsed.size(); ++i)
                block.addDirective(parsed[i]);
            return block;
        }
        else if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string blockLine = accumulated;
            std::string childName = blockLine.substr(0, blockLine.size() - 1);
            childName = trimLine(childName);
            BlockParser child = parseBlock(file, childName, lineNumber);
            block.addNest(child);
            accumulated.clear();
        }
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            accumulated = accumulated.substr(0, accumulated.size() - 1);
            std::vector<std::string> tokens = tokenize(accumulated, lineNumber);
            if (!parser.parseDirective(tokens, lineNumber))
            {
                std::stringstream message3;
                message3 << "⚠️ Error parsing directive: " << trimmed << " at line: " << lineNumber << "\n";
                throw std::runtime_error(message3.str());
            }
            accumulated.clear();
        }
    }
    if (!accumulated.empty())
    {
        std::stringstream message4;
        message4 << "⚠️ Error: Unterminated directive at EOF \n"
                << "  Start at line: " << directiveStartLine << "\n  Content: " << accumulated << "\n";
        throw std::runtime_error(message4.str());
    }
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
        std::cout << "  Directive [" << i << "] (line " << directives[i].lineNumber << "): " << std::endl;
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