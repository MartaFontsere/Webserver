#include "../../../includes/config_parser/parser/DirectiveParser.hpp"
#include <cstdlib>
#include <iostream>


DirectiveParser::DirectiveParser()
{

}

DirectiveParser::DirectiveParser(const DirectiveParser &other)
{
    (void)other;
}

// Operador de asignación
DirectiveParser &DirectiveParser::operator=(const DirectiveParser &other)
{
    (void)other;
    return *this;
}

// Destructor
DirectiveParser::~DirectiveParser()
{

}

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

const std::vector<DirectiveToken> &DirectiveParser::getDirectives() const
{
    return _directives;
}