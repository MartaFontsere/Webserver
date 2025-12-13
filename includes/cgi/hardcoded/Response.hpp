#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response
{
private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    Response();
    Response(int code, const std::string &message);

    int getStatusCode() const;
    std::string getStatusMessage() const;
    std::string getHeader(const std::string &key) const;
    std::string getBody() const;
    std::string toString() const;

    void setStatusCode(int code);
    void setStatusMessage(const std::string &msg);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);
    void appendBody(const std::string &content);
};

#endif