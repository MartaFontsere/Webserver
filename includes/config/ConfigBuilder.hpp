#ifndef CONFIGBUILDER_HPP
#define CONFIGBUILDER_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "UtilsConfig.hpp"
#include "../config_parser/parser/BlockParser.hpp"

class ConfigBuilder
{
private:
    ServerConfig buildServer(const BlockParser &serverBlock);
    LocationConfig buildLocation(const BlockParser &locationBlock);

    std::string getDirectiveValue(const BlockParser &block, const std::string &directiveName);
    std::vector<std::string> getDirectiveValues(const BlockParser &block, const std::string &directiveName);
    int getDirectiveValueAsInt(const BlockParser &block, const std::string &directiveName);

public:
    ConfigBuilder();
    ~ConfigBuilder();
    std::vector<ServerConfig> buildFromBlockParser(const BlockParser &root);
};

#endif