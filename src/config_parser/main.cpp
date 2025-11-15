#include <iostream>
#include <fstream>
#include <string>
#include "../../includes/config_parser/DirectiveParser.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/BlockParser.hpp"
#include "../../includes/config_parser/ValidationStructureConfig.hpp"

int main()
{
    std::cout << "=== TESTING CONFIG PARSER ===" << std::endl;
    if (!validationStructureConfigFile("test.conf"))
    {
        std::cerr << "Error canalla! " << std::endl;
        return 0;
    }
    std::cerr << " 🏅 Mu bien!!!! 🏅" << std::endl;
    // try
    // {
    //     // BlockParser root = readConfigFile("test.conf");
    //     // root.printBlock(root);
    //     BlockParser configRoot = readConfigFile("test.conf");
    //     const std::vector<BlockParser> &topLevelBlocks = configRoot.getNestedBlocks();
    //     for (size_t i = 0; i < topLevelBlocks.size(); ++i)
    //     {
    //         // Si printBlock NO es estático (es método de instancia):
    //         configRoot.printBlock(topLevelBlocks[i]);
    //         // Si printBlock es estático:
    //         // BlockParser::printBlock(topLevelBlocks[i]);
    //     }
    // }
    // catch (std::exception &e)
    // {
    //     std::cerr << "Error: " << e.what() << std::endl;
    // }
    return 0;
}
