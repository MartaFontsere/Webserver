#include <iostream>
#include <fstream>
#include <string>
#include "../../includes/config_parser/DirectiveParser.hpp"
#include "../../includes/config_parser/UtilsConfigParser.hpp"
#include "../../includes/config_parser/BlockParser.hpp"
#include "../../includes/config_parser/ValidationStructureConfig.hpp"

int main()
{
    try
    {
        std::cout << "=== TESTING CONFIG PARSER ===" << std::endl;
        validationStructureConfigFile("test.conf");
        std::cerr << " 🏅 Mu bien!!!! 🏅" << std::endl;
        // BlockParser root = readConfigFile("test.conf");
        // root.printBlock(root);
        BlockParser configRoot = readConfigFile("test.conf");
        const std::vector<BlockParser> &topLevelBlocks = configRoot.getNestedBlocks();
        for (size_t i = 0; i < topLevelBlocks.size(); ++i)
        {
            // Si printBlock NO es estático (es método de instancia):
            configRoot.printBlock(topLevelBlocks[i]);
            // Si printBlock es estático:
            // BlockParser::printBlock(topLevelBlocks[i]);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // TEST DEL TOKENIZADOR
    // std::string test1 = "root \"/var/www/my site\"";
    // std::vector<std::string> tokens1 = tokenize(test1);
    // std::cout << "\nTest 1: " << test1 << std::endl;
    // for (size_t i = 0; i < tokens1.size(); i++)
    //     std::cout << "  Token[" << i << "]: '" << tokens1[i] << "'" << std::endl;

    // std::string test2 = "server_name 'example.com' localhost";
    // std::vector<std::string> tokens2 = tokenize(test2);
    // std::cout << "\nTest 2: " << test2 << std::endl;
    // for (size_t i = 0; i < tokens2.size(); i++)
    //     std::cout << "  Token[" << i << "]: '" << tokens2[i] << "'" << std::endl;

    return 0;
}
