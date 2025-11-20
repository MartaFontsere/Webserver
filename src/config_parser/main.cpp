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
        std::cout << "=== TESTING CONFIG PARSER ===" << std::endl;
        validationStructureConfigFile("test.conf");

        BlockParser root = readConfigFile("test.conf");

        SemanticValidator validator;
        bool valid = validator.validate(root);
        if (!valid)
        {
            validator.printReport();
            return 1;
        }
        std::cout << "✅ Configuración válida" << std::endl;

        const std::vector<BlockParser> &topLevelBlocks = root.getNestedBlocks();
        for (size_t i = 0; i < topLevelBlocks.size(); ++i)
            root.printBlock(topLevelBlocks[i]);
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
