#ifndef DIRECTIVEPARSER_HPP
#define DIRECTIVEPARSER_HPP

#include <string>
#include <vector>

// Definición del struct ServerConfig
struct ServerConfig
{
    int listenPort;
    std::string serverName;
    std::string root;
};

class DirectiveParser
{
public:
    // Orthodox Canonical Form (OCF)
    DirectiveParser();                                        // Constructor por defecto
    DirectiveParser(const DirectiveParser &other);            // Constructor de copia
    DirectiveParser &operator=(const DirectiveParser &other); // Operador de asignación
    ~DirectiveParser();                                       // Destructor

    // Método principal
    bool parseDirective(ServerConfig &config, const std::vector<std::string> &tokens);

    // Métodos de validación
    bool isValidDirective(const std::string &directive);

private:
    // Métodos auxiliares privados
    bool parseListenDirective(ServerConfig &config, const std::string &value);
    bool parseServerNameDirective(ServerConfig &config, const std::string &value);
    bool parseRootDirective(ServerConfig &config, const std::string &value);
};

#endif