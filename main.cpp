// #include "Server.hpp"
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "../includes/config/ConfigBuilder.hpp"

int main(int argc, char **argv)
{
    std::string configPath;
    if (argc == 1)
        configPath = "test.conf";
    else
        configPath = argv[1];

    //  Server server("8080");
    try
    {
        BlockParser root = parseAndValidateConfig(configPath);
        ConfigBuilder builder;
        std::vector<ServerConfig> servers = builder.buildFromBlockParser(root);

        std::cout << "✅ Configuration loaded: " << servers.size() << " server(s)" << std::endl;

        // if (!server.init())
        // {
        //     return 1;
        // }

        // server.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "❌ Config error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
