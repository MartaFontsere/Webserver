#ifndef UTILSCONFIGPARSER_HPP
#define UTILSCONFIGPARSER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include "DirectiveParser.hpp"
#include "BlockParser.hpp"
#include "../validation/ValidationStructureConfig.hpp"
#include "../validation/SemanticValidator.hpp"

std::string trimLine(const std::string &line);

bool isEmptyOrComment(const std::string &trimmedLine);

std::vector<std::string> tokenize(const std::string &line, int numLine);

BlockParser readConfigFile(const std::string &filePath);

int initConfigParser(const std::string &configPath);

#endif