#ifndef DIRECTIVEPARSER_HPP
#define DIRECTIVEPARSER_HPP

#include <string>
#include <vector>

struct DirectiveToken
{
    std::string name;
    std::vector<std::string> values;
};

class DirectiveParser
{
private:
    std::vector<DirectiveToken> _directives;

public:
    // Orthodox Canonical Form (OCF)
    DirectiveParser();                                        // Constructor por defecto
    DirectiveParser(const DirectiveParser &other);            // Constructor de copia
    DirectiveParser &operator=(const DirectiveParser &other); // Operador de asignación
    ~DirectiveParser();                                       // Destructor

    // parsea tokens en una directive y la guarda internamente
    bool parseDirective(const std::vector<std::string> &tokens);

    // Imprime las directivas almacenadas en el parser
    void printDirectives() const;

    // Acceso (si quieres usarlo desde fuera)
    const std::vector<DirectiveToken> &getDirectives() const;
};

#endif