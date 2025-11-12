#include <iostream>
#include <fstream>
#include <string>
#include "DirectiveParser.hpp"
#include "UtilsConfigParser.hpp"
#include "BlockParser.hpp"

int main()
{
    std::cout << "=== TESTING CONFIG PARSER ===" << std::endl;

    try
    {
        // BlockParser root = readConfigFile("test.conf");
        // root.printBlock(root);
        BlockParser configRoot = readConfigFile("test.conf");

        // 1. Obtener los bloques de nivel superior (los hijos del 'root' vacío)
        const std::vector<BlockParser> &topLevelBlocks = configRoot.getNestedBlocks();

        // 2. Iterar e imprimir CADA UNO de ellos
        for (size_t i = 0; i < topLevelBlocks.size(); ++i)
        {
            // Llama a la impresión usando el bloque hijo, no el 'root'.
            // Usamos 'topLevelBlocks[i]' como argumento, que es el bloque 'server' o 'paquito'.

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
    return 0;
}
