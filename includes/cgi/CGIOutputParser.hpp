#ifndef CGIOUTPUTPARSER_HPP
#define CGIOUTPUTPARSER_HPP

#include <string>
#include <sstream>
#include <map>

class CGIOutputParser
{
private:
    std::map<std::string, std::string> _headers;
    std::string _body;
    int _statusCode;

public:
    CGIOutputParser();
    ~CGIOutputParser();

    void parse(const std::string &rawOutput);

    std::map<std::string, std::string> getHeaders() const;
    std::string getBody() const;
    int getStatusCode() const;
};

#endif