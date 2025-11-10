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
bool DirectiveParser::parseDirective(ServerConfig &config, const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        return false; // Necesitamos al menos comando + valor
    }

    std::string command = tokens[0]; // "listen", "server_name", etc.
    std::string value = tokens[1];   // "8080", "localhost", etc.

    if (command == "listen")
        return parseListenDirective(config, value);
    else if (command == "server_name")
        return parseServerNameDirective(config, value);
    else if (command == "root")
        return parseRootDirective(config, value);
    return false;
}

// Métodos de validación
bool DirectiveParser::isValidDirective(const std::string &directive)
{
    if (directive == "listen")
        return true;
    else if (directive == "server_name")
        return true;
    else if (directive == "root")
        return true;
    return false;
}

// Métodos auxiliares privados
bool DirectiveParser::parseListenDirective(ServerConfig &config, const std::string &value)
{
    config.listenPort = atoi(value.c_str());
    return true;
}
bool DirectiveParser::parseServerNameDirective(ServerConfig &config, const std::string &value)
{
    config.serverName = value;
    return true;
}
bool DirectiveParser::parseRootDirective(ServerConfig &config, const std::string &value)
{
    config.root = value;
    return true;
}