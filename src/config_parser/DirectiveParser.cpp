#include "DirectiveParser.hpp"
#include <cstdlib>
#include <iostream>

// Constructor por defecto
DirectiveParser::DirectiveParser()
{
    // No hay atributos que inicializar - clase stateless
}

// Constructor de copia
DirectiveParser::DirectiveParser(const DirectiveParser &other)
{
    // No hay atributos que copiar
    (void)other; // Evita warning de parámetro no usado
}

// Operador de asignación
DirectiveParser &DirectiveParser::operator=(const DirectiveParser &other)
{
    // No hay atributos que asignar
    (void)other; // Evita warning
    return *this;
}

// Destructor
DirectiveParser::~DirectiveParser()
{
    // No hay recursos que liberar
}

// Método principal
// recogemos los toquens para guardarlos en el struct
bool DirectiveParser::parseDirective(const std::vector<std::string> &tokens)
{
    if (tokens.empty())
        return false;

    DirectiveToken directive;
    directive.name = tokens[0];

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