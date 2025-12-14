#include "Server.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "../includes/config/ConfigBuilder.hpp"
int main()
{
    //  Server server("8080");
    // TODO; CONTROLAR ACCESO DE CONFIG CON Y SIN ARGUMENTO
    // Configuration file path (relative to executable location)
    const std::string configPath = "src/test.conf";

    try
    {
        BlockParser root = parseAndValidateConfig(configPath);
        ConfigBuilder builder;
        std::vector<ServerConfig> servers = builder.buildFromBlockParser(root);

        // Verificación básica
        std::cout << "\n=== CONFIG LOADED SUCCESSFULLY ===" << std::endl;
        std::cout << "Total servers: " << servers.size() << std::endl;

        // Verificar que hay al menos 1 server
        if (servers.size() > 0)
        {
            std::cout << "\n--- Server 0 Details ---" << std::endl;
            std::cout << "Port: " << servers[0].getListen() << std::endl;
            std::cout << "Host: " << servers[0].getHost() << std::endl;
            std::cout << "Root: " << servers[0].getRoot() << std::endl;
            std::cout << "Max Body Size: " << servers[0].getClientMaxBodySize() << " bytes" << std::endl;
            std::cout << "Locations: " << servers[0].getLocations().size() << std::endl;

            // Mostrar server_names si hay
            std::vector<std::string> names = servers[0].getServerNames();
            if (names.size() > 0)
            {
                std::cout << "Server names: ";
                for (size_t i = 0; i < names.size(); ++i)
                {
                    std::cout << names[i];
                    if (i < names.size() - 1)
                        std::cout << ", ";
                }
                std::cout << std::endl;
            }
        }
        std::cout << "===================================\n"
                  << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "❌ Config error: " << e.what() << std::endl;
        return 1;
    }
    // if (!server.init())
    // {
    //     return 1;
    // }

    // server.run();

    return 0;
}