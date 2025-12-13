#include "Server.hpp"
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
int main()
{
    //  Server server("8080");

    // Configuration file path (relative to executable location)
    const std::string configPath = "src/test.conf";

    // Parse and validate configuration
    if (initConfigParser(configPath) != 0)
    {
        std::cerr << "❌ Failed to parse configuration file: " << configPath << std::endl;
        return 1;
    }
    // if (!server.init())
    // {
    //     return 1;
    // }

    // server.run();

    return 0;
}