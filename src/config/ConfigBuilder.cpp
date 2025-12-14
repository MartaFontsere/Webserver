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

void ConfigBuilder::parseAutoindex(const BlockParser &locationBlock, LocationConfig &location)
{
    std::string autoindexValue = getDirectiveValue(locationBlock, "autoindex");
    if (autoindexValue == "on")
        location.setAutoindex(true);
    else
        location.setAutoindex(false);
}
void ConfigBuilder::parseReturn(const BlockParser &locationBlock, LocationConfig &location)
{
    std::vector<std::string> returnValues = getDirectiveValues(locationBlock, "return");
    if (returnValues.size() >= 2)
    {
        int valueInt = 0;
        valueInt = stringToInt(returnValues[0]);
        location.setReturnCode(valueInt);
        location.setReturnUrl(returnValues[1]);
    }
    else if (returnValues.size() == 1)
    {
        int valueInt = 0;
        valueInt = stringToInt(returnValues[0]);
        location.setReturnCode(valueInt);
    }
    else
        location.setReturnCode(0);
}
void ConfigBuilder::parseErrorPages(const BlockParser &locationBlock, LocationConfig &location)
{
    std::vector<DirectiveToken> directives = locationBlock.getDirectives();
    std::map<int, std::string> errorMap;
    for (size_t i = 0; i < directives.size(); ++i)
    {
        if (directives[i].name == "error_page")
        {
            if (directives[i].values.size() >= 2)
            {
                int code = stringToInt(directives[i].values[0]);
                std::string path = directives[i].values[1];
                errorMap[code] = path;
            }
        }
    }
    location.setErrorPages(errorMap);
}

LocationConfig ConfigBuilder::buildLocation(const BlockParser &locationBlock)
{
    LocationConfig location;

    location.setPattern(locationBlock.getName());
    location.setRoot(getDirectiveValue(locationBlock, "root"));
    location.setIndex(getDirectiveValues(locationBlock, "index"));
    location.setBodySize(getDirectiveValueAsInt(locationBlock, "client_max_body_size"));
    location.setCgiExts(getDirectiveValues(locationBlock, "cgi_ext"));
    location.setCgiPaths(getDirectiveValues(locationBlock, "cgi_path"));
    location.setMethods(getDirectiveValues(locationBlock, "allow_methods"));
    location.setUploadPath(getDirectiveValue(locationBlock, "upload_path"));

    parseAutoindex(locationBlock, location);
    parseReturn(locationBlock, location);
    parseErrorPages(locationBlock, location);

    return location;
}

std::vector<ServerConfig> buildFromBlockParser(const BlockParser &root);

ServerConfig buildServer(const BlockParser &serverBlock);
