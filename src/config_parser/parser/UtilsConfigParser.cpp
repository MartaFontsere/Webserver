#include "../../../includes/config_parser/parser/UtilsConfigParser.hpp"

/**
 * @brief Removes leading and trailing whitespace from a string
 *
 * Trims spaces, tabs, carriage returns, and newlines from both ends
 * of the string. Returns empty string if the line contains only whitespace.
 *
 * @param line String to trim
 * @return Trimmed string, or empty string if only whitespace
 */
std::string trimLine(const std::string &line)
{
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

/**
 * @brief Checks if a line is empty or a comment
 *
 * A line is considered empty or comment if:
 * - It contains only whitespace, OR
 * - It starts with '#' (after trimming)
 *
 * @param trimmedLine Line to check (should be pre-trimmed for efficiency)
 * @return true if empty or comment, false otherwise
 */
bool isEmptyOrComment(const std::string &trimmedLine)
{
    std::string temp = trimLine(trimmedLine);
    return temp.empty() || temp[0] == '#';
}

/**
 * @brief Tokenizes a line into words, respecting quoted strings
 *
 * Splits a line by spaces and tabs, but preserves quoted strings as single tokens.
 * Handles both single quotes (') and double quotes (").
 * Quotes are removed from the resulting tokens.
 *
 * Examples:
 *   "listen 8080" → ["listen", "8080"]
 *   "root '/var/www/my site'" → ["root", "/var/www/my site"]
 *   "error_page 404 '/not found.html'" → ["error_page", "404", "/not found.html"]
 *
 * @param line Line to tokenize (without trailing semicolon)
 * @param numLine Line number (for error reporting)
 * @return Vector of tokens (strings)
 * @throw std::runtime_error if quotes are not closed properly
 */
std::vector<std::string> tokenize(const std::string &line, int numLine)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < line.size(); ++i)
    {
        char temp = line[i];
        // Opening quote
        if (!inQuotes && (temp == '"' || temp == '\''))
        {
            inQuotes = true;
            quoteChar = temp;
            continue;
        }
        // Closing quote
        if (inQuotes && temp == quoteChar)
        {
            inQuotes = false;
            quoteChar = '\0';
            continue;
        }
        // Separator (outside quotes)
        if (!inQuotes && (temp == ' ' || temp == '\t'))
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current += temp;
    }
    // Add last token
    if (!current.empty())
        tokens.push_back(current);

    // Verify quotes are closed
    if (inQuotes)
    {
        std::stringstream message;
        message << "Warning: Unclosed quote in line: " << numLine;
        throw std::runtime_error(message.str());
    }
    return tokens;
}

/**
 * @brief Reads and parses a complete nginx-style configuration file
 *
 * Main entry point for parsing. Reads the file line by line, handling:
 * - Multi-line directives (accumulated until ';' is found)
 * - Nested blocks (recursive calls to BlockParser::parseBlock)
 * - Comments (full-line and inline, stripped before processing)
 * - Whitespace normalization
 *
 * The function builds a complete configuration tree (BlockParser) containing
 * all directives and nested blocks found in the file.
 *
 * Process:
 * 1. Read file line by line
 * 2. Trim and skip empty/comment lines
 * 3. Strip inline comments
 * 4. Accumulate multi-line content until ';' or '{'
 * 5. Parse directives (ending with ';')
 * 6. Parse nested blocks (ending with '{') recursively
 * 7. Return complete tree at EOF
 *
 * @param filePath Path to the configuration file to read
 * @return BlockParser object representing the root of the config tree
 * @throw std::runtime_error if file cannot be opened or syntax errors are found
 */
BlockParser readConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        throw std::runtime_error("❌ File can't be open");

    BlockParser root;
    std::string line;
    std::string accumulated;
    int lineNumber = 0;
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

        if (accumulated.empty())
            directiveStartLine = lineNumber;

        if (!accumulated.empty())
            accumulated += " ";
        accumulated += trimmed;
        // Block opening
        if (trimmed[trimmed.size() - 1] == '{')
        {
            std::string blockLine = accumulated;
            std::string blockName = blockLine.substr(0, blockLine.size() - 1);
            blockName = trimLine(blockName);
            BlockParser temp;
            BlockParser nest = temp.parseBlock(file, blockName, lineNumber);
            root.addNest(nest);
            accumulated.clear();
        }
        // Directive ending
        else if (trimmed[trimmed.size() - 1] == ';')
        {
            DirectiveParser parser;
            accumulated = accumulated.substr(0, accumulated.size() - 1);
            std::vector<std::string> tokens = tokenize(accumulated, lineNumber);
            parser.parseDirective(tokens, directiveStartLine);
            const std::vector<DirectiveToken> &dirs = parser.getDirectives();
            for (size_t i = 0; i < dirs.size(); ++i)
                root.addDirective(dirs[i]);
            accumulated.clear();
        }
    }
    // Verify no unterminated directive at EOF
    if (!accumulated.empty())
    {
        std::stringstream message;
        message << "⚠️ Error: Unterminated directive at EOF \n"
                << "  Started at line: " << directiveStartLine << "\n  Content: " << accumulated;
        throw std::runtime_error(message.str());
    }
    return root;
}

/**
 * @brief Initializes and executes the complete configuration parsing pipeline
 *
 * Main entry point for the configuration parser module. Orchestrates the
 * complete validation and parsing process in three phases:
 *
 * === PARSING PIPELINE ===
 *
 * PHASE 1: Structural Validation (StructuralValidator)
 *   - Validates balanced braces { }
 *   - Checks semicolon placement
 *   - Verifies block syntax correctness
 *   - Detects unclosed blocks/directives
 *   Result: Early failure if file structure is malformed
 *
 * PHASE 2: Syntactic Parsing (BlockParser + DirectiveParser)
 *   - Reads and tokenizes configuration file
 *   - Builds hierarchical block tree
 *   - Parses directives and their arguments
 *   - Handles nested blocks (http, server, location)
 *   Result: Complete configuration tree (BlockParser)
 *
 * PHASE 3: Semantic Validation (SemanticValidator)
 *   - Validates directive names and contexts
 *   - Checks argument counts and types
 *   - Verifies required directives present
 *   - Detects invalid configurations
 *   Result: Semantically correct configuration ready for use
 *
 * Error handling:
 * - Structural errors: Stops immediately, prints all errors found
 * - Parsing errors: Throws runtime_error with line number
 * - Semantic errors: Generates report with all violations
 *
 * Output:
 * - Success: "✅ Valid configuration"
 * - Failure: Detailed error report with line numbers and explanations
 *
 * Example usage:
 *   int result = initConfigParser("config/webserv.conf");
 *   if (result != 0) {
 *       std::cerr << "Configuration parsing failed" << std::endl;
 *       return 1;
 *   }
 *   // Configuration is valid, proceed with server startup
 *
 * @param configPath Path to nginx-style configuration file to parse
 * @return 0 on success (configuration valid), 1 on failure (errors found)
 *
 * @note Prints error messages to stderr and success message to stdout
 * @note Does not modify any global state - only validates the configuration
 * @see validateStructure() for Phase 1 (structural validation)
 * @see readConfigFile() for Phase 2 (parsing)
 * @see SemanticValidator::validate() for Phase 3 (semantic validation)
 */
int initConfigParser(const std::string &configPath)
{
    try
    {
        // PHASE 1: Structural validation (braces, semicolons)
        std::vector<std::string> structuralErrors;
        if (!validateStructure(configPath, structuralErrors))
        {
            // Display structural errors with formatting
            std::cerr << "❌ Structural validation failed with "
                      << structuralErrors.size() << " error(s):" << std::endl;
            for (size_t i = 0; i < structuralErrors.size(); ++i)
                std::cerr << structuralErrors[i] << std::endl;
            return 1;
        }
        // PHASE 2: Parse configuration into tree structure
        BlockParser root = readConfigFile("src/test.conf");
        // PHASE 3: Semantic validation (directives, contexts, types)
        SemanticValidator validator;
        if (!validator.validate(root))
        {
            validator.printReport();
            return 1;
        }

        std::cout << "✅ Configuración válida" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
