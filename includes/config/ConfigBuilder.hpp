#ifndef CONFIGBUILDER_HPP
#define CONFIGBUILDER_HPP

#include "../config_parser/parser/BlockParser.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "UtilsConfig.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Configuration builder - converts BlockParser tree to typed configs
 */
class ConfigBuilder {
private:
  ServerConfig buildServer(const BlockParser &serverBlock);
  LocationConfig buildLocation(const BlockParser &locationBlock);

  std::string getDirectiveValue(const BlockParser &block,
                                const std::string &directiveName);
  std::vector<std::string> getDirectiveValues(const BlockParser &block,
                                              const std::string &directiveName);
  int getDirectiveValueAsInt(const BlockParser &block,
                             const std::string &directiveName);

  void parseAutoindex(const BlockParser &locationBlock,
                      LocationConfig &location);
  void parseReturn(const BlockParser &locationBlock, LocationConfig &location);
  void locationParseErrorPages(const BlockParser &locationBlock,
                               LocationConfig &location);
  void serverParseErrorPages(const BlockParser &serverBlock,
                             ServerConfig &server);
  void serverParseLocation(const BlockParser &serverBlock,
                           ServerConfig &server);

public:
  ConfigBuilder();
  ~ConfigBuilder();

  /** @brief Build ServerConfigs from parsed configuration tree */
  std::vector<ServerConfig> buildFromBlockParser(const BlockParser &root);
};

#endif