#include "../../../includes/config_parser/parser/DirectiveParser.hpp"
#include <cstdlib>
#include <iostream>

/**
 * @brief Default constructor - creates an empty directive parser
 *
 * Initializes a DirectiveParser with an empty directives vector.
 */
DirectiveParser::DirectiveParser()
{
}

/**
 * @brief Copy constructor (unused - suppressed for compilation)
 *
 * Not implemented as DirectiveParser doesn't require deep copying.
 * Parameter suppressed to avoid unused parameter warnings.
 *
 * @param other DirectiveParser to copy from (unused)
 */
DirectiveParser::DirectiveParser(const DirectiveParser &other)
{
    (void)other;
}

/**
 * @brief Assignment operator (unused - suppressed for compilation)
 *
 * Not implemented as DirectiveParser doesn't require assignment.
 * Parameter suppressed to avoid unused parameter warnings.
 *
 * @param other DirectiveParser to assign from (unused)
 * @return Reference to this object (*this)
 */
DirectiveParser &DirectiveParser::operator=(const DirectiveParser &other)
{
    (void)other;
    return *this;
}

/**
 * @brief Destructor - cleans up resources
 *
 * Default destructor. The directives vector cleans itself automatically.
 */
DirectiveParser::~DirectiveParser()
{
}

/**
 * @brief Gets all parsed directives
 *
 * Returns a const reference to the internal directives vector.
 * This allows read-only access without copying the entire vector.
 *
 * @return Const reference to vector of DirectiveToken objects
 */
const std::vector<DirectiveToken> &DirectiveParser::getDirectives() const
{
    return _directives;
}

/**
 * @brief Parses a tokenized directive and stores it
 *
 * Creates a DirectiveToken from a vector of tokens where:
 * - tokens[0] is the directive name (e.g., "listen", "root")
 * - tokens[1..n] are the directive arguments/values
 *
 * The parsed directive is added to the internal directives vector.
 *
 * Example:
 *   tokens = ["listen", "8080", "default_server"]
 *   → Creates DirectiveToken { name="listen", values=["8080", "default_server"], lineNumber=X }
 *
 * @param tokens Vector of strings (first = name, rest = values)
 * @param lineNum Line number in the config file where directive appears
 * @return true if parsing succeeded, false if tokens is empty
 */
bool DirectiveParser::parseDirective(const std::vector<std::string> &tokens, int lineNum)
{
    if (tokens.empty())
        return false;

    DirectiveToken directive;
    directive.name = tokens[0];
    directive.lineNumber = lineNum;

    if (tokens.size() > 1)
    {
        for (size_t i = 1; i < tokens.size(); ++i)
            directive.values.push_back(tokens[i]);
    }

    _directives.push_back(directive);
    return true;
}
/**
 * @brief Prints all parsed directives to stdout (for debugging)
 *
 * Displays each directive with its name and values in a readable format.
 * Useful for debugging the parsing process and verifying directive extraction.
 *
 * Output format:
 *   Directiva #1: name = listen, values = 8080, default_server
 *   Directiva #2: name = root, values = /var/www/html
 */
void DirectiveParser::printDirectives() const
{
    for (size_t i = 0; i < _directives.size(); ++i)
    {
        std::cout << "Directiva #" << i + 1 << ": "
                  << "name = " << _directives[i].name
                  << ", values = ";

        if (_directives[i].values.empty())
            std::cout << "(none)";
        else
        {
            for (size_t j = 0; j < _directives[i].values.size(); ++j)
            {
                if (j > 0)
                    std::cout << ", ";
                std::cout << _directives[i].values[j];
            }
        }
        std::cout << std::endl;
    }
}
