#ifndef UTILSCONFIGPARSER_HPP
#define UTILSCONFIGPARSER_HPP

#include <string>
#include <vector>
#include "DirectiveParser.hpp"
#include "BlockParser.hpp"

std::string trimLine(const std::string &line);

bool isEmptyOrComment(const std::string &trimmedLine);

std::vector<std::string> tokenize(const std::string &line);

BlockParser readConfigFile(const std::string &filePath);

#endif