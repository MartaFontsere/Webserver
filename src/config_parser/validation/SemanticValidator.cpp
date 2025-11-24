#include "../../../includes/config_parser/validation/SemanticValidator.hpp"
#include "../../../includes/config_parser/validation/ValueValidator.hpp"
#include <iostream>
#include <sstream>

/**
 * @brief Default constructor - creates an empty semantic validator
 *
 * Initializes a SemanticValidator with empty error and warning vectors.
 */
SemanticValidator::SemanticValidator()
{
}

/**
 * @brief Gets all accumulated validation errors
 *
 * Returns a const reference to the errors vector. Errors are accumulated
 * during validation (not fail-fast) to provide complete error reporting.
 *
 * @return Const reference to vector of error messages
 */
const std::vector<std::string> &SemanticValidator::getErrors() const
{
    return _errors;
}

/**
 * @brief Gets all accumulated validation warnings
 *
 * Returns a const reference to the warnings vector. Currently unused
 * but available for future non-critical issues.
 *
 * @return Const reference to vector of warning messages
 */
const std::vector<std::string> &SemanticValidator::getWarnings() const
{
    return _warnings;
}

/**
 * @brief Checks if any errors were found during validation
 *
 * @return true if errors exist, false if validation was clean
 */
bool SemanticValidator::hasErrors() const
{
    return !_errors.empty();
}

/**
 * @brief Clears all accumulated errors and warnings
 *
 * Resets the validator state. Useful for reusing the same validator
 * object for multiple validation runs.
 */
void SemanticValidator::clear()
{
    _errors.clear();
    _warnings.clear();
}

/**
 * @brief Maps a block name to its corresponding Context enum
 *
 * Determines the context type based on block name:
 * - "http" → CTX_HTTP
 * - "server" → CTX_SERVER
 * - "location ..." → CTX_LOCATION (prefix match)
 * - "events" → CTX_EVENTS
 * - "" (root) or unknown → CTX_MAIN
 *
 * Note: "location" blocks include their pattern in the name
 * (e.g., "location /", "location ~ \.php$"), so we use prefix matching.
 *
 * @param blockName Name of the block (can include arguments like "location /api")
 * @return Context enum corresponding to the block type
 */
Context SemanticValidator::getBlockContext(const std::string &blockName) const
{
    if (blockName == "http")
        return CTX_HTTP;
    if (blockName == "server")
        return CTX_SERVER;
    if (blockName.find("location ") == 0)
        return CTX_LOCATION;
    if (blockName == "events")
        return CTX_EVENTS;
    return CTX_MAIN;
}

/**
 * @brief Validates a single directive in a given context
 *
 * Performs complete validation of a directive:
 * 1. Checks if directive exists (DirectiveMetadata::getRule)
 * 2. Validates context (DirectiveMetadata::isValidInContext)
 * 3. Validates arguments (DirectiveMetadata::validateArguments - 3 levels)
 *
 * Errors are accumulated (not fail-fast). If directive doesn't exist,
 * stops checking context/arguments (no rule to validate against).
 * If directive exists but context is wrong, still validates arguments
 * to provide complete error information.
 *
 * @param directive DirectiveToken containing name, values, and line number
 * @param ctx Context where the directive appears (CTX_HTTP, CTX_SERVER, etc.)
 */
void SemanticValidator::validateDirective(const DirectiveToken &directive, Context ctx)
{
    const DirectiveRule *rule = DirectiveMetadata::getRule(directive.name);
    if (rule == NULL)
    {
        std::stringstream message;
        message << "Error line " << directive.lineNumber << ": Unknown directive '"
                << directive.name << "'";
        _errors.push_back(message.str());
        return;
    }
    // Validate context
    if (!DirectiveMetadata::isValidInContext(directive.name, ctx))
    {
        std::stringstream message;
        message << "Error line " << directive.lineNumber
                << ": Directive '" << directive.name
                << "' not allowed in this context";
        _errors.push_back(message.str());
    }
    // Validate arguments (continue even if context failed)
    if (!DirectiveMetadata::validateArguments(directive.name, directive.values))
    {
        std::stringstream message;
        message << "Error line " << directive.lineNumber
                << ": Invalid Arguments for '" << directive.name << "'";
        _errors.push_back(message.str());
    }
}

/**
 * @brief Recursively validates a configuration block and all its children
 *
 * Performs multi-level validation:
 * 1. Block nesting rules (http in MAIN, server in HTTP, location in SERVER, etc.)
 * 2. Unknown block detection
 * 3. Location pattern validation (if block is location)
 * 4. All directives within the block (via validateDirective)
 * 5. All nested child blocks (recursive call)
 *
 * Context handling:
 * - parentCtx: Context where this block appears (used to validate nesting)
 * - blockCtx: Context that this block CREATES (used to validate its directives)
 *
 * Example:
 *   validateBlock(http, CTX_MAIN)
 *     parentCtx = CTX_MAIN (http appears at root)
 *     blockCtx = CTX_HTTP (http creates HTTP context)
 *     → Validates: http can be in MAIN ✅
 *     → Validates directives with CTX_HTTP
 *     → Validates children with CTX_HTTP as parent
 *
 * Nesting rules (nginx-compliant):
 * - http: must be at root level (CTX_MAIN)
 * - server: must be inside http (CTX_HTTP)
 * - location: must be inside server (CTX_SERVER)
 * - events: must be at root level (CTX_MAIN)
 *
 * @param block BlockParser object to validate
 * @param parentCtx Context where this block appears (parent's context)
 */
void SemanticValidator::validateBlock(const BlockParser &block, Context parentCtx)
{
    std::string blockName = block.getName();
    Context blockCtx = getBlockContext(blockName);
    if (!blockName.empty())
    {
        bool isKnown = false;
        // Validate http block
        if (blockName == "http")
        {
            isKnown = true;
            if (parentCtx != CTX_MAIN)
            {
                std::stringstream message;
                message << "Error line " << block.getStartLine()
                        << ": 'http' block not allowed here (must be at root level)";
                _errors.push_back(message.str());
            }
        }
        // Validate server block
        else if (blockName == "server")
        {
            isKnown = true;
            if (parentCtx != CTX_HTTP)
            {
                std::stringstream message;
                message << "Error line " << block.getStartLine()
                        << ": 'server' block not allowed here (must be inside 'http')";
                _errors.push_back(message.str());
            }
        }
        // Validate events block
        else if (blockName == "events")
        {
            isKnown = true;
            if (parentCtx != CTX_MAIN)
            {
                std::stringstream message;
                message << "Error line " << block.getStartLine()
                        << ": 'events' block not allowed here (must be at root level)";
                _errors.push_back(message.str());
            }
        }
        // Validate location block
        else if (blockName.find("location ") == 0)
        {
            isKnown = true;
            if (parentCtx != CTX_SERVER)
            {
                std::stringstream message;
                message << "Error line " << block.getStartLine()
                        << ": 'location' block not allowed here (must be inside 'server')";
                _errors.push_back(message.str());
            }
            // Validate location pattern
            std::string pattern = blockName.substr(9); // After "location "
            if (!isValidPattern(pattern))
            {
                std::stringstream message;
                message << "Error line " << block.getStartLine()
                        << ": Invalid location pattern '" << pattern << "'";
                _errors.push_back(message.str());
            }
        }
        // Unknown block detection
        if (!isKnown)
        {
            std::stringstream message;
            message << "Error line " << block.getStartLine()
                    << ": Unknown block '" << blockName << "'";
            _errors.push_back(message.str());
        }
    }
    // Validate all directives in this block
    std::vector<DirectiveToken> directives = block.getDirectives();
    for (size_t i = 0; i < directives.size(); ++i)
    {
        validateDirective(directives[i], blockCtx); // Use block's own context
    }
    // Recursively validate all nested blocks
    std::vector<BlockParser> children = block.getNestedBlocks();
    for (size_t i = 0; i < children.size(); ++i)
    {
        validateBlock(children[i], blockCtx); // Block becomes parent of children
    }
}

/**
 * @brief Validates a complete configuration tree (main entry point)
 *
 * Performs comprehensive semantic validation:
 * 1. Empty configuration detection
 * 2. Mandatory block validation (http OR events required)
 * 3. Recursive validation of entire tree
 * 4. Error accumulation (not fail-fast)
 *
 * Validation process:
 * 1. Clear previous state
 * 2. Check if config is empty
 * 3. Validate entire tree recursively (starting at CTX_MAIN)
 * 4. Verify at least one mandatory block exists (http or events)
 * 5. Return validation result
 *
 * The validator accumulates ALL errors found during traversal,
 * not stopping at the first error. This provides complete
 * error reporting in a single validation pass.
 *
 * @param rootParser Root BlockParser representing entire configuration
 * @return true if configuration is valid, false if errors were found
 */
bool SemanticValidator::validate(const BlockParser &rootParser)
{
    clear();
    // Check for empty configuration
    if (rootParser.getDirectives().empty() && rootParser.getNestedBlocks().empty())
    {
        _errors.push_back("Error: Configuration file is empty");
        return false;
    }
    // Validate entire tree
    validateBlock(rootParser, CTX_MAIN);
    // Verify mandatory blocks (nginx requirement)
    std::vector<BlockParser> children = rootParser.getNestedBlocks();
    bool hasHttp = false;
    bool hasEvents = false;
    for (size_t i = 0; i < children.size(); ++i)
    {
        if (children[i].getName() == "http")
            hasHttp = true;
        if (children[i].getName() == "events")
            hasEvents = true;
    }
    if (!hasHttp && !hasEvents)
    {
        _errors.push_back("Error: Configuration must contain at least 'http' or 'events block'");
        return false;
    }
    return !hasErrors();
}

/**
 * @brief Prints a formatted validation report to stdout/stderr
 *
 * Output format:
 * - If valid: "✅ Configuration is valid" (to stdout)
 * - If errors: Lists all errors with line numbers (to stderr)
 * - If warnings: Lists all warnings after errors (to stderr)
 *
 * Example error output:
 *   ❌ Configuration validation failed with 3 error(s):
 *
 *   Error line 5: Unknown directive 'liste'
 *   Error line 12: Directive 'listen' not allowed in this context
 *   Error line 20: Invalid arguments for 'root'
 *
 * Designed for professional CLI tool output.
 */
void SemanticValidator::printReport() const
{
    if (_errors.empty())
    {
        std::cout << "✅ Configuration is valid" << std::endl;
        return;
    }

    std::cerr << "❌ Configuration validation failed with "
              << _errors.size() << " error(s):" << std::endl;
    std::cerr << std::endl;

    for (size_t i = 0; i < _errors.size(); ++i)
    {
        std::cerr << _errors[i] << std::endl;
    }

    if (!_warnings.empty())
    {
        std::cerr << std::endl;
        std::cerr << "⚠️  " << _warnings.size() << " warning(s):" << std::endl;
        for (size_t i = 0; i < _warnings.size(); ++i)
        {
            std::cerr << _warnings[i] << std::endl;
        }
    }
}