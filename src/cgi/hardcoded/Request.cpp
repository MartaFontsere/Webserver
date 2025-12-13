#include "../../../includes/cgi/hardcoded/Request.hpp"

Request::Request() : _method("GET"), _uri("/"), _protocol("HTTP/1.1")
{
}

Request::Request(const std::string &method, const std::string &uri)
    : _method(method), _uri(uri), _protocol("HTTP/1.1")
{
}

std::string Request::getMethod() const
{
    return _method;
}

std::string Request::getURI() const
{
    return _uri;
}

std::string Request::getProtocol() const
{
    return _protocol;
}

std::string Request::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}

std::string Request::getBody() const
{
    return _body;
}

std::map<std::string, std::string> Request::getHeaders() const
{
    return _headers;
}

void Request::setMethod(const std::string &method)
{
    _method = method;
}

void Request::setURI(const std::string &uri)
{
    _uri = uri;
}

void Request::setProtocol(const std::string &protocol)
{
    _protocol = protocol;
}

void Request::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void Request::setBody(const std::string &body)
{
    _body = body;
}

std::string Request::getQueryString() const
{
    size_t pos = _uri.find('?');
    if (pos == std::string::npos)
        return "";
    return _uri.substr(pos + 1);
}

std::string Request::getPathInfo() const
{
    size_t pos = _uri.find('?');
    if (pos == std::string::npos)
        return _uri;
    return _uri.substr(0, pos);
}

std::string Request::getScriptName() const
{
    std::string path = getPathInfo();
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos)
        return path;
    return path.substr(lastSlash + 1);
}