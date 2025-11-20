#include <iostream>
#include <fstream>
#include <string>
#include "../../includes/config_parser/DirectiveParser.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/BlockParser.hpp"
#include "../../includes/config_parser/ValidationStructureConfig.hpp"
#include "../../includes/config_parser/SemanticValidator.hpp"

int main()
{
    try
    {
        std::vector<std::string> structuralErrors;
        if (!validateStructure("test.conf", structuralErrors))
        {
            // Mostrar errores estructurales
            std::cerr << "❌ Structural validation failed with " 
                    << structuralErrors.size() << " error(s):" << std::endl;
            for (size_t i = 0; i < structuralErrors.size(); ++i)
                std::cerr << structuralErrors[i] << std::endl;
            return 1;
        }
        BlockParser root = readConfigFile("test.conf");
        SemanticValidator validator;
        if (!validator.validate(root))
        {
            validator.printReport();
            return 1;
        }

        std::cout << "✅ Configuración válida" << std::endl;

    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
