#include <iostream>
#include <fstream>
#include <string>
#include "DirectiveParser.hpp"
#include "UtilsConfigParser.hpp"

int main()
{
    std::cout << "=== TESTING CONFIG PARSER ===" << std::endl;

    DirectiveToken config; // Crear config aquí

    // Por ahora solo probamos con un archivo de ejemplo
    bool result = readConfigFile("test.conf"); // Pasar config

    if (result)
    {
        std::cout << "✅ Archivo leído correctamente" << std::endl;

        // DEBUG SIMPLE - Mostrar config
        std::cout << "\n=== CONFIGURACIÓN PARSEADA ===" << std::endl;
        /* std::cout << "Puerto: " << config.listenPort << std::endl;
         std::cout << "Server Name: " << config.serverName << std::endl;
         std::cout << "Root: " << config.root << std::endl;*/
    }
    else
    {
        std::cout << "❌ Error al leer archivo" << std::endl;
    }

    return 0;
}
