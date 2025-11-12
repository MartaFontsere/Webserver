#include "DirectiveParser.hpp"
#include <cstdlib>

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
    if (tokens.size() < 2)
    {
        return false; // Necesitamos al menos comando + valor
    }

    DirectiveToken directive;
    directive.name = tokens[0];  // "listen", "server_name", etc.
    directive.value = tokens[1]; // "8080", "localhost", etc.
    return false;
}