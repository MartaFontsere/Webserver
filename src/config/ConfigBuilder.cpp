#include "../../includes/config/ConfigBuilder.hpp"

ConfigBuilder::ConfigBuilder()
{
}

ConfigBuilder::~ConfigBuilder()
{
}

std::string ConfigBuilder::getDirectiveValue(const BlockParser &block, const std::string &directiveName)
{
    std::vector<DirectiveToken> directives = block.getDirectives();

    for (size_t i = 0; i < directives.size(); ++i)
    {
        if (directives[i].name == directiveName)
        {
            if (!directives[i].values.empty())
            {
                return directives[i].values[0];
            }
        }
    }
    return "";
}

std::vector<std::string> ConfigBuilder::getDirectiveValues(const BlockParser &block, const std::string &directiveName)
{
    std::vector<DirectiveToken> directives = block.getDirectives();

    for (size_t i = 0; i < directives.size(); ++i)
    {
        if (directives[i].name == directiveName)
        {
            if (!directives[i].values.empty())
            {
                return directives[i].values;
            }
        }
    }
    return std::vector<std::string>();
}

int ConfigBuilder::getDirectiveValueAsInt(const BlockParser &block, const std::string &directiveName)
{
    std::string directive = getDirectiveValue(block, directiveName);
    if (!directive.empty())
    {
        int value = stringToInt(directive);
        return value;
    }
    return 0;
}

std::vector<ServerConfig> buildFromBlockParser(const BlockParser &root);

ServerConfig buildServer(const BlockParser &serverBlock);
LocationConfig buildLocation(const BlockParser &locationBlock);
