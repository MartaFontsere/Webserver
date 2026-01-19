#ifndef UTILSCONFIGPARSER_HPP
#define UTILSCONFIGPARSER_HPP

#include "../validation/SemanticValidator.hpp"
#include "../validation/ValidationStructureConfig.hpp"
#include "BlockParser.hpp"
#include "DirectiveParser.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string trimLine(const std::string &line);
bool isEmptyOrComment(const std::string &trimmedLine);
std::vector<std::string> tokenize(const std::string &line, int numLine);
BlockParser readConfigFile(const std::string &filePath);
int initConfigParser(const std::string &configPath);

/** @brief Main entry point - parse and validate config file, throw on error */
BlockParser parseAndValidateConfig(const std::string &configPath);

#endif