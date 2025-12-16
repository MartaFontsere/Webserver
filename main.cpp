// #include "Server.hpp"
#include "includes/config_parser/parser/UtilsConfigParser.hpp"
#include "includes/config/ServerConfig.hpp"
#include "includes/cgi/CGIHandler.hpp"
#include "includes/cgi/hardcoded/Request.hpp" // ← Mock (temporal)
#include "includes/config/LocationConfig.hpp" // ← Real del Config Parser
#include "includes/response/HttpResponse.hpp"
#include "includes/config/ConfigBuilder.hpp"

#include <iostream>
// int main(int argc, char **argv)
int main()
{
    // std::string configPath;
    // if (argc == 1)
    //     configPath = "test.conf";
    // else
    //     configPath = argv[1];

    // //  Server server("8080");
    // try
    // {
    //     BlockParser root = parseAndValidateConfig(configPath);
    //     ConfigBuilder builder;
    //     std::vector<ServerConfig> servers = builder.buildFromBlockParser(root);

    //     std::cout << "✅ Configuration loaded: " << servers.size() << " server(s)" << std::endl;

    //     // if (!server.init())
    //     // {
    //     //     return 1;
    //     // }

    //     // server.run();
    // }
    // catch (std::exception &e)
    // {
    //     std::cerr << "❌ Config error: " << e.what() << std::endl;
    //     return 1;
    // }

    std::cout << "=== Test CGI with HttpResponse ===" << std::endl;

    // Setup mock request
    Request req;
    req.setURI("/hello.php?name=test");
    req.setMethod("GET");

    // Setup LocationConfig usando SETTERS
    LocationConfig loc;
    loc.setRoot("./src/cgi/test_scripts");

    std::vector<std::string> cgiPaths;
    cgiPaths.push_back("/usr/bin/php-cgi");
    loc.setCgiPaths(cgiPaths);

    std::vector<std::string> cgiExts;
    cgiExts.push_back(".php");
    loc.setCgiExts(cgiExts);

    // ✅ Setup ServerConfig (AÑADIR ESTO):
    ServerConfig server;
    server.setListen(8080);

    std::vector<std::string> serverNames;
    serverNames.push_back("localhost");
    server.setServerNames(serverNames);

    // ⚠️ PROBLEMA: serverName y serverPort NO existen en LocationConfig
    // Solución temporal: CGIEnvironment usa valores por defecto

    // Execute CGI
    CGIHandler handler;
    HttpResponse response = handler.handle(req, loc, server);

    // Print HTTP response
    std::cout << "\n📤 HTTP Response from CGI:\n"
              << std::endl;
    std::cout << response.buildResponse() << std::endl;

    return 0;
}
