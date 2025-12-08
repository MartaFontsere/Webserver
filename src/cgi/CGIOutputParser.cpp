#include "../../includes/cgi/CGIOutputParser.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

CGIOutputParser::CGIOutputParser()
{
    _statusCode = 0;
}

CGIOutputParser::~CGIOutputParser()
{
}

void CGIOutputParser::parse(const std::string &rawOutput)
{
    size_t pos = rawOutput.find("\r\n\r\n");

    std::string headersSection = rawOutput.substr(0, pos);
    std::string bodySection = rawOutput.substr(pos + 4);

    _body = bodySection;

    std::istringstream stream(headersSection);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line[line.size() - 1] == '\r')
            line.erase(line.size() - 1, 1);
        size_t colonPos = line.find(":");
        std::string keyLine = line.substr(0, colonPos);
        std::string valueLine = line.substr(colonPos + 2);
        _headers[keyLine] = valueLine;
    }
}

int CGIOutputParser::getStatusCode() const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find("Status");
    if (it != _headers.end())
    {
        std::string statusValue = it->second;
        std::string codeStr = statusValue.substr(0, 3);
        std::stringstream ss(codeStr);
        int code;
        ss >> code;

        return code;
    }
    return 200;
}

std::map<std::string, std::string> CGIOutputParser::getHeaders() const
{
    return _headers;
}

std::string CGIOutputParser::getBody() const
{
    return _body;
}