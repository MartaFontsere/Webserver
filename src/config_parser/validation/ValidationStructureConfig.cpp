#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cctype>
#include <sstream>
#include "../../../includes/config_parser/validation/ValidationStructureConfig.hpp"
#include "../../../includes/config_parser/parser/UtilsConfigParser.hpp"

/**
 * @brief Validates that braces and semicolons are not orphaned
 *
 * Checks if a line starts with '{' or ';' without a preceding name/directive.
 * This catches syntax errors like:
 *   {           ← Invalid (no block name)
 *   ;           ← Invalid (no directive name)
 *
 * Errors are accumulated (not thrown) for complete error reporting.
 *
 * @param trimmedLine Line to check (already trimmed)
 * @param lineCont Line number in the file
 * @param filePath Path to the config file (for error messages)
 * @param errors Vector to accumulate error messages
 */
void checkEmptyBraceOrSemicolon(const std::string &trimmedLine, int lineCont,
                                const std::string &filePath,
                                std::vector<std::string> &errors)
{
    if (trimmedLine[0] == '{')
    {
        std::stringstream message;
        message << "Error line " << lineCont
                << ": No name before '{' in " << filePath;
        errors.push_back(message.str());
    }
    else if (trimmedLine[0] == ';')
    {
        std::stringstream message;
        message << "Error line " << lineCont
                << ": No name before ';' in " << filePath;
        errors.push_back(message.str());
    }
}

/**
 * @brief Counts opening braces and tracks the first occurrence
 *
 * Increments the open brace counter if the line ends with '{'.
 * Records the line number of the FIRST opening brace found
 * (used for error reporting if braces are unbalanced).
 *
 * @param trimmedLine Line to check (already trimmed)
 * @param lineCont Current line number (for tracking first occurrence)
 * @param contOpenKey Reference to total open brace counter (incremented)
 * @return Line number of first '{' if this is the first, 0 otherwise
 */
int contOpenKeys(const std::string &trimmedLine, int &lineCont, int &contOpenKey)
{
    int firstOpenKey = 0;
    if (trimmedLine[trimmedLine.size() - 1] == '{')
    {
        contOpenKey++;
        if (contOpenKey == 1)
            firstOpenKey = lineCont;
    }
    return firstOpenKey;
}

/**
 * @brief Counts closing braces and tracks the last occurrence
 *
 * Increments the close brace counter if the line ends with '}'.
 * Records the line number of EVERY closing brace found
 * (the last one is used for error reporting if extra braces exist).
 *
 * @param trimmedLine Line to check (already trimmed)
 * @param lineCont Current line number (for tracking last occurrence)
 * @param contCloseKey Reference to total close brace counter (incremented)
 * @return Line number if line ends with '}', 0 otherwise
 */
int contCloseKeys(const std::string &trimmedLine, int &lineCont, int &contCloseKey)
{
    int lastCloseKey = 0;
    if (trimmedLine[trimmedLine.size() - 1] == '}')
    {
        contCloseKey++;
        lastCloseKey = lineCont;
    }
    return lastCloseKey;
}

/**
 * @brief Processes a config line and updates brace tracking state
 *
 * Calls contOpenKeys and contCloseKeys to count braces, then updates
 * the tracking variables for first opening and last closing braces.
 * These are used later to provide accurate error messages if braces
 * are unbalanced.
 *
 * @param trimmedLine Line to process (already trimmed)
 * @param lineCont Current line number
 * @param contOpenKey Reference to total open brace counter
 * @param contCloseKey Reference to total close brace counter
 * @param firstOpenKey Reference to first open brace line (updated if first)
 * @param lastCloseKey Reference to last close brace line (updated each time)
 */
void processConfigLine(const std::string &trimmedLine, int &lineCont, int &contOpenKey,
                       int &contCloseKey, int &firstOpenKey, int &lastCloseKey)
{
    int openLine = contOpenKeys(trimmedLine, lineCont, contOpenKey);
    int closeLine = contCloseKeys(trimmedLine, lineCont, contCloseKey);
    if (openLine > 0 && firstOpenKey == 0)
        firstOpenKey = openLine;
    if (closeLine > 0)
        lastCloseKey = closeLine;
}

/**
 * @brief Checks if a character is allowed in nginx-style config files
 *
 * Allowed characters:
 * - Alphanumeric (a-z, A-Z, 0-9)
 * - Path characters: / . _ -
 * - Special: : * , = @ $ " ' (quotes)
 * - Whitespace: space tab
 * - Syntax: ; { }
 * - Comments: #
 * - Regex: ~ ^ \ | ( ) [ ] + ?
 *
 * Note: Regex characters added to support location patterns
 * like "location ~ \.php$" or "location ^~ /static/".
 *
 * @param character Character to validate
 * @return true if character is allowed, false otherwise
 */
static bool isValidConfigChar(char character)
{
    unsigned char temp = static_cast<unsigned char>(character);

    if (std::isalnum(temp))
        return true;

    switch (character)
    {
    case '/':
    case '.':
    case '_':
    case '-':
    case ':':
    case '*':
    case ',':
    case '=':
    case '@':
    case '$':
    case '"':
    case '\'':
    case ' ':
    case '\t':
    case ';':
    case '{':
    case '}':
    case '#':
    case '~':
    case '^':
    case '\\':
    case '|':
    case '(':
    case ')':
    case '[':
    case ']':
    case '+':
    case '?':
        return true;

    default:
        return false;
    }
}

/**
 * @brief Validates all characters in a line against allowed character set
 *
 * Scans the entire line character by character. If an invalid character
 * is found, adds an error and returns immediately (reports only first
 * invalid character per line).
 *
 * @param trimmedLine Line to validate (already trimmed)
 * @param lineCont Line number in the file
 * @param errors Vector to accumulate error messages
 */
void checkInvalidCharacters(const std::string &trimmedLine, int lineCont, std::vector<std::string> &errors)
{
    for (size_t i = 0; i < trimmedLine.size(); ++i)
    {
        if (!isValidConfigChar(trimmedLine[i]))
        {
            std::stringstream message;
            message << "Error line " << lineCont
                    << ": Invalid character '" << trimmedLine[i] << "'";
            errors.push_back(message.str());
            return;
        }
    }
}

/**
 * @brief Validates that opening and closing braces are balanced
 *
 * Checks three conditions:
 * - contOpenKey > contCloseKey → Missing '}' (error at first '{')
 * - contOpenKey < contCloseKey → Extra '}' (error at last '}')
 * - contOpenKey == contCloseKey → Balanced (OK)
 *
 * @param contOpenKey Total count of opening braces '{'
 * @param contCloseKey Total count of closing braces '}'
 * @param firstOpenKey Line number of first '{'
 * @param lastCloseKey Line number of last '}'
 * @param filePath Path to config file (unused, suppressed)
 * @param errors Vector to accumulate error messages
 */
void checkBraceBalance(int contOpenKey, int contCloseKey, int firstOpenKey,
                       int lastCloseKey, const std::string &filePath, std::vector<std::string> &errors)
{
    (void)filePath; // No necesitamos filePath
    if (contOpenKey > contCloseKey)
    {
        std::stringstream message;
        message << "Error line " << firstOpenKey
                << ": Missing closing brace '}'";
        errors.push_back(message.str());
    }
    else if (contOpenKey < contCloseKey)
    {
        std::stringstream message;
        message << "Error line " << lastCloseKey
                << ": Unexpected closing brace '}'";
        errors.push_back(message.str());
    }
}

/**
 * @brief Validates the structural correctness of a config file
 *
 * Performs low-level syntax validation BEFORE parsing:
 * 1. File accessibility
 * 2. Character whitelist validation
 * 3. Orphaned braces/semicolons detection
 * 4. Brace balance verification
 * 5. Comment handling (full-line and inline)
 *
 * This is a lightweight pre-flight check. It does NOT validate:
 * - Directive semantics (handled by SemanticValidator)
 * - Block nesting rules (handled by SemanticValidator)
 * - Argument types (handled by DirectiveMetadata + ValueValidator)
 *
 * Validation process:
 * 1. Open file
 * 2. Read line by line
 * 3. Skip empty lines and comments
 * 4. Strip inline comments
 * 5. Check for valid characters
 * 6. Check for orphaned { or ;
 * 7. Track brace counts
 * 8. Verify brace balance at EOF
 *
 * All errors are accumulated (not fail-fast) to provide complete
 * error reporting in a single pass.
 *
 * @param filePath Path to the configuration file to validate
 * @param errors Vector to accumulate error messages
 * @return true if structure is valid, false if errors were found
 */
bool validateStructure(const std::string &filePath, std::vector<std::string> &errors)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
    {
        errors.push_back("Error: Cannot open file '" + filePath + "'");
        return false;
    }
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
            // Strip inline comments
            size_t commentPos = trimmed.find('#');
            if (commentPos != std::string::npos)
            {
                trimmed = trimmed.substr(0, commentPos);
                trimmed = trimLine(trimmed);
            }
            if (!trimmed.empty())
            {
                checkEmptyBraceOrSemicolon(trimmed, lineCont, filePath, errors);
                checkInvalidCharacters(trimmed, lineCont, errors);
                processConfigLine(trimmed, lineCont, contOpenKey, contCloseKey,
                                  firstOpenKey, lastCloseKey);
            }
        }
        lineCont++;
    }
    // Final brace balance check
    checkBraceBalance(contOpenKey, contCloseKey, firstOpenKey, lastCloseKey,
                      filePath, errors);
    return errors.empty();
}
