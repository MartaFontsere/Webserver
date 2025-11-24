#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class Request
{
private:
    std::string _method;
    std::string _uri;
    std::string _protocol;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    Request();
    Request(const std::string &method, const std::string &uri);

    std::string getMethod() const;
    std::string getURI() const;
    std::string getProtocol() const;
    std::string getHeader(const std::string &key) const;
    std::string getBody() const;
    std::map<std::string, std::string> getHeaders() const;

    void setMethod(const std::string &method);
    void setURI(const std::string &uri);
    void setProtocol(const std::string &protocol);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);

    std::string getQueryString() const;
    std::string getPathInfo() const;
    std::string getScriptName() const;
};

#endif