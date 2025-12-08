#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

struct LocationConfig
{
    std::string root;
    std::vector<std::string> cgiPaths;
    std::vector<std::string> cgiExts;
    std::string serverName;
    int serverPort;

    LocationConfig()
    {
        root = "./test_scripts";
        cgiPaths.push_back("/usr/bin/php-cgi");
        cgiExts.push_back(".php");
        cgiExts.push_back(".php5");
        cgiExts.push_back(".phtml");
        serverName = "localhost";
        serverPort = 8080;
    }
};

#endif