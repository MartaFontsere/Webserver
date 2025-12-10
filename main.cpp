#include "Server.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include "../includes/config_parser/parser/DirectiveParser.hpp"
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "../includes/config_parser/parser/BlockParser.hpp"
#include "../includes/config_parser/validation/ValidationStructureConfig.hpp"
#include "../includes/config_parser/validation/SemanticValidator.hpp"

int main()
{
    Server server("8080");
    try
    {
        std::vector<std::string> structuralErrors;
        if (!validateStructure("src/test.conf", structuralErrors))
        {
            // Mostrar errores estructurales
            std::cerr << "❌ Structural validation failed with "
                      << structuralErrors.size() << " error(s):" << std::endl;
            for (size_t i = 0; i < structuralErrors.size(); ++i)
                std::cerr << structuralErrors[i] << std::endl;
            return 1;
        }
        BlockParser root = readConfigFile("src/test.conf");
        SemanticValidator validator;
        if (!validator.validate(root))
        {
            validator.printReport();
            return 1;
        }

        std::cout << "✅ Configuración válida" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // if (!server.init())
    // {
    //     return 1;
    // }

    // server.run();

    return 0;
}