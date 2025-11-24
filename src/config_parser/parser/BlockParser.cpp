#include "../../../includes/config_parser/parser/BlockParser.hpp"
#include "../../../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "../../../includes/config_parser/validation/DirectiveMetadata.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

/**
 * @brief Default constructor - creates an empty block
 *
 * Initializes a BlockParser with empty name and line numbers set to 0.
 */

BlockParser::BlockParser() : startLine(0), endLine(0)
{
}
/**
 * @brief Constructor with block name and starting line
 *
 * Creates a BlockParser for a named block (e.g., "http", "server", "location /").
 *
 * @param blockName Name of the block (can include arguments like "location /api")
 * @param start Line number where the block starts in the config file
 */
BlockParser::BlockParser(const std::string &blockName, int start) : name(blockName), startLine(start), endLine(0) {};

/**
 * @brief Assignment operator - deep copy of block data
 *
 * Copies name, directives, and nested blocks from another BlockParser.
 * Implements self-assignment protection.
 *
 * @param other BlockParser to copy from
 * @return Reference to this object (*this)
 */
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

/**
 * @brief Destructor - cleans up resources
 *
 * Default destructor. Vectors clean themselves automatically.
 */
BlockParser::~BlockParser()
{
}

/**
 * @brief Gets the name of the block
 *
 * @return Block name (e.g., "http", "server", "location /api")
 */
std::string BlockParser::getName() const
{
    return name;
}

/**
 * @brief Gets all directives contained in this block
 *
 * Returns a copy of the directives vector. Does not include directives
 * from nested blocks.
 *
 * @return Vector of DirectiveToken objects
 */
std::vector<DirectiveToken> BlockParser::getDirectives() const
{
    return directives;
}

/**
 * @brief Gets all nested blocks (children) of this block
 *
 * Returns a copy of the nested blocks vector. For recursive traversal
 * of the entire configuration tree.
 *
 * @return Vector of BlockParser objects
 */
std::vector<BlockParser> BlockParser::getNestedBlocks() const
{
    return nestedBlocks;
}

/**
 * @brief Gets the line number where the block starts
 *
 * @return Starting line number in the config file
 */
int BlockParser::getStartLine() const
{
    return startLine;
}

/**
 * @brief Gets the line number where the block ends
 *
 * The ending line is the line with the closing brace '}'.
 *
 * @return Ending line number in the config file
 */
int BlockParser::getEndLine() const
{
    return endLine;
}

/**
 * @brief Sets the name of the block
 *
 * @param newName New name for the block
 */
void BlockParser::setName(const std::string &newName)
{
    name = newName;
}

/**
 * @brief Sets the ending line number
 *
 * Called when the closing brace '}' of the block is found.
 *
 * @param line Line number where the block ends
 */
void BlockParser::setEndLine(int line)
{
    endLine = line;
}
/**
 * @brief Adds a directive to this block
 *
 * Appends a parsed directive to the directives vector.
 *
 * @param directive DirectiveToken to add (contains name, values, lineNumber)
 */
void BlockParser::addDirective(const DirectiveToken &directive)
{
    directives.push_back(directive);
}

/**
 * @brief Adds a nested block to this block
 *
 * Appends a child block to the nestedBlocks vector.
 * Used for building the hierarchical config tree.
 *
 * @param nest BlockParser object representing the nested block
 */
void BlockParser::addNest(const BlockParser &nest)
{
    nestedBlocks.push_back(nest);
}

/**
 * @brief Checks if a line starts with a known directive
 *
 * Extracts the first word from the line and verifies if it matches
 * any directive registered in DirectiveMetadata. Used to detect
 * unterminated directives (missing semicolon).
 *
 * @param line Trimmed line to check
 * @return true if line starts with a known directive, false otherwise
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

/**
 * @brief Recursively parses a configuration block from a file stream
 *
 * Reads lines from the file until the closing brace '}' is found.
 * Handles:
 * - Multi-line directives (accumulated until ';')
 * - Nested blocks (recursive parseBlock calls)
 * - Inline and full-line comments (stripped out)
 * - Syntax validation (unterminated directives, missing semicolons)
 *
 * The function modifies the lineNumber reference to track the current
 * position in the file during recursive parsing.
 *
 * @param file Input file stream (must be open and readable)
 * @param blockName Name of the block being parsed (e.g., "server", "location /")
 * @param lineNumber Reference to current line number (modified during parsing)
 * @return BlockParser object containing all parsed directives and nested blocks
 * @throw std::runtime_error if syntax errors are detected (unterminated directives, etc.)
 */
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
        // Strip inline comments
        size_t commentPos = trimmed.find('#');
        if (commentPos != std::string::npos)
        {
            trimmed = trimmed.substr(0, commentPos);
            trimmed = trimLine(trimmed);
        }

        if (trimmed.empty())
            continue;
        // Detect unterminated directive (missing semicolon)
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
        // Nested block opening
        else if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string blockLine = accumulated;
            std::string childName = blockLine.substr(0, blockLine.size() - 1);
            childName = trimLine(childName);
            BlockParser child = parseBlock(file, childName, lineNumber);
            block.addNest(child);
            accumulated.clear();
        }
        // Directive ending
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
    // Reached EOF with unterminated directive
    if (!accumulated.empty())
    {
        std::stringstream message4;
        message4 << "⚠️ Error: Unterminated directive at EOF \n"
                 << "  Start at line: " << directiveStartLine << "\n  Content: " << accumulated << "\n";
        throw std::runtime_error(message4.str());
    }
    return block;
}

/**
 * @brief Recursively prints the entire block structure (for debugging)
 *
 * Displays block name, line range, all directives with their values,
 * and recursively prints all nested blocks. Useful for debugging
 * the parsing process and visualizing the configuration tree.
 *
 * @param block BlockParser object to print
 */
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