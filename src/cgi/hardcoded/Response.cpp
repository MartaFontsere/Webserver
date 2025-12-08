#include "../../../includes/cgi/hardcoded/Response.hpp"
#include <sstream>

Response::Response() : _statusCode(200), _statusMessage("OK")
{
}

Response::Response(int code, const std::string &body)
    : _statusCode(code), _body(body)
{
}

int Response::getStatusCode() const
{
    return _statusCode;
}

std::string Response::getStatusMessage() const
{
    return _statusMessage;
}

std::string Response::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}

std::string Response::getBody() const
{
    return _body;
}

std::string Response::toString() const
{
    std::stringstream ss;

    ss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

    std::map<std::string, std::string>::const_iterator it;
    for (it = _headers.begin(); it != _headers.end(); ++it)
    {
        ss << it->first << ": " << it->second << "\r\n";
    }

    ss << "\r\n";
    ss << _body;

    return ss.str();
}

void Response::setStatusCode(int code)
{
    _statusCode = code;
}

void Response::setStatusMessage(const std::string &msg)
{
    _statusMessage = msg;
}

void Response::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void Response::setBody(const std::string &body)
{
    _body = body;
}

void Response::appendBody(const std::string &content)
{
    _body += content;
}