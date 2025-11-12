#include <string>
#include <vector>
#include "DirectiveParser.hpp"
#include "BlockParser.hpp"

std::string trimLine(const std::string &line);

bool isEmptyOrComment(const std::string &trimmedLine);

std::vector<std::string> split(const std::string &str, char delimiter);

BlockParser readConfigFile(const std::string &filePath);