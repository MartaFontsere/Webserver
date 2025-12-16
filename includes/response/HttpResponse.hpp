#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>

class HttpResponse
{
private:
    int _statusCode;
    std::string _statusMessage;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    HttpResponse();
    ~HttpResponse();

    HttpResponse(const HttpResponse &other);
    HttpResponse &operator=(const HttpResponse &other);

    void setStatus(int code, const std::string &message);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);

    std::string buildResponse();

    void setErrorResponse(int code);
};

#endif
