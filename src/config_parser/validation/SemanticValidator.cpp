#include "../../../includes/config_parser/validation/SemanticValidator.hpp"
#include "../../../includes/config_parser/validation/ValueValidator.hpp"
#include <iostream>
#include <sstream>

SemanticValidator::SemanticValidator()
{
}

const std::vector<std::string> &SemanticValidator::getErrors() const
{
    return _errors;
}

const std::vector<std::string> &SemanticValidator::getWarnings() const
{
    return _warnings;
}

bool SemanticValidator::hasErrors() const
{
    return !_errors.empty();
}

void SemanticValidator::clear()
{
    _errors.clear();
    _warnings.clear();
}

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

    if (!DirectiveMetadata::isValidInContext(directive.name, ctx))
    {
        std::stringstream message;
        message << "Error line " << directive.lineNumber
                << ": Directive '" << directive.name
                << "' not allowed in this context";
        _errors.push_back(message.str());
    }
    if (!DirectiveMetadata::validateArguments(directive.name, directive.values))
    {
        std::stringstream message;
        message << "Error line " << directive.lineNumber
                << ": Invalid Arguments for '" << directive.name << "'";
        _errors.push_back(message.str());
    }
}

void SemanticValidator::validateBlock(const BlockParser &block, Context parentCtx)
{
    std::string blockName = block.getName();
    Context blockCtx = getBlockContext(blockName);
    if (!blockName.empty())
    {
        bool isKnown = false;

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
            std::string pattern = blockName.substr(9);
            if (!isValidPattern(pattern))
            {
                std::stringstream message;
                message << "Error line " << block.getStartLine()
                        << ": Invalid location pattern '" << pattern << "'";
                _errors.push_back(message.str());
            }
        }

        if (!isKnown)
        {
            std::stringstream message;
            message << "Error line " << block.getStartLine()
                    << ": Unknown block '" << blockName << "'";
            _errors.push_back(message.str());
        }
    }
    std::vector<DirectiveToken> directives = block.getDirectives();
    for (size_t i = 0; i < directives.size(); ++i)
    {
        validateDirective(directives[i], blockCtx);
    }
    std::vector<BlockParser> children = block.getNestedBlocks();
    for (size_t i = 0; i < children.size(); ++i)
    {
        validateBlock(children[i], blockCtx);
    }
}

bool SemanticValidator::validate(const BlockParser &rootParser)
{
    clear();
    if (rootParser.getDirectives().empty() && rootParser.getNestedBlocks().empty())
    {
        _errors.push_back("Error: Configuration file is empty");
        return false;
    }
    validateBlock(rootParser, CTX_MAIN);
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