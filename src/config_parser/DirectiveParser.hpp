#ifndef DIRECTIVEPARSER_HPP
#define DIRECTIVEPARSER_HPP

#include <string>
#include <vector>

// Definición del struct DirectiveToken
/*struct DirectiveToken
{
    int listenPort;
    std::string serverName;
    std::string root;
};*/

struct DirectiveToken
{
    std::string name;
    std::string value;
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
    bool parseDirective(const std::vector<std::string> &tokens);

private:
    // Métodos auxiliares privados
    bool parseListenDirective(DirectiveToken &directive, const std::string &value);
    bool parseServerNameDirective(DirectiveToken &config, const std::string &value);
    bool parseRootDirective(DirectiveToken &config, const std::string &value);
};

#endif