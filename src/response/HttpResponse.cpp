#include "../../includes/response/HttpResponse.hpp"
#include "../../includes/response/UtilsResponse.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

HttpResponse::HttpResponse()
    : _statusCode(200),
      _statusMessage("OK"),
      _httpVersion("HTTP/1.1")
{
}

HttpResponse::HttpResponse(const HttpResponse &other)
    : _statusCode(other._statusCode),
      _statusMessage(other._statusMessage),
      _httpVersion(other._httpVersion),
      _headers(other._headers),
      _body(other._body)
{
}

HttpResponse &HttpResponse::operator=(const HttpResponse &other)
{
    if (this != &other)
    {
        _statusCode = other._statusCode;
        _statusMessage = other._statusMessage;
        _httpVersion = other._httpVersion;
        _headers = other._headers;
        _body = other._body;
    }
    return *this;
}

HttpResponse::~HttpResponse()
{
}

std::string HttpResponse::getMimeType(const std::string &path) const
{
    size_t lastDot = path.find_last_of(".");
    if (lastDot != std::string::npos)
    {
        std::map<std::string, std::string> mimeTypes;
        mimeTypes[".html"] = "text/html";
        mimeTypes[".htm"] = "text/html";
        mimeTypes[".css"] = "text/css";
        mimeTypes[".txt"] = "text/plain";
        mimeTypes[".jpg"] = "image/jpeg";
        mimeTypes[".jpeg"] = "image/jpeg";
        mimeTypes[".png"] = "image/png";
        mimeTypes[".gif"] = "image/gif";
        mimeTypes[".svg"] = "image/svg+xml";
        mimeTypes[".php"] = "text/x-php";
        mimeTypes[".py"] = "text/x-python";
        mimeTypes[".sh"] = "application/x-sh";
        mimeTypes[".json"] = "application/json";
        mimeTypes[".js"] = "application/javascript";
        std::map<std::string, std::string>::const_iterator it = mimeTypes.find(path.substr(lastDot));
        if (it != mimeTypes.end())
            return it->second;
        else
            return "application/octet-stream";
    }
    return "application/octet-stream";
}

std::string HttpResponse::getStatusMessage(int code) const
{
    switch (code)
    {
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 413:
        return "Request Entity Too Large";
    case 500:
        return "Internal Server Error";
    default:
        return "Internal Server Error";
    }
}

void HttpResponse::setContentTypeFromPath(const std::string &path)
{
    setHeader("Content-Type", getMimeType(path));
}

void HttpResponse::setStatus(int code, const std::string &message)
{
    _statusCode = code;
    _statusMessage = message;
}

void HttpResponse::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void HttpResponse::setBody(const std::string &body)
{
    _body = body;
}

void HttpResponse::setErrorResponse(int code)
{
    _httpVersion = "HTTP/1.1";
    _statusCode = code;
    _statusMessage = getStatusMessage(code);
    switch (code)
    {
    case 403:
        _body = "<html><body><h1>403 Forbidden</h1></body></html>";
        break;
    case 404:
        _body = "<html><body><h1>404 Not Found</h1></body></html>";
        break;
    case 405:
        _body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        break;
    case 413:
        _body = "<html><body><h1>413 Request Entity Too Large</h1>"
                "<p>Maximum body size is 10MB</p></body></html>";
        break;
    case 500:
    default:
        _body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    }

    _headers["Content-Type"] = "text/html";
    _headers["Content-Length"] = sizeToString(_body.size());
}

std::string HttpResponse::readErrorFile(const std::string &path) const
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        std::cerr << "❌ Error: No se pudo abrir " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

void HttpResponse::setErrorResponse(int code, const std::map<int, std::string> &errorPages)
{
    _httpVersion = "HTTP/1.1";
    _statusCode = code;

    std::map<int, std::string>::const_iterator it = errorPages.find(code);
    if (it != errorPages.end())
    {
        std::string errorContent = readErrorFile(it->second);
        if (!errorContent.empty())
        {
            _httpVersion = "HTTP/1.1";
            _statusCode = code;
            _statusMessage = getStatusMessage(code);
            _body = errorContent;
            _headers["Content-Type"] = "text/html";
            _headers["Content-Length"] = sizeToString(_body.size());
            return;
        }
    }
    setErrorResponse(code);
}

std::string HttpResponse::buildResponse()
{
    setHeader("Server", "webserv/1.0");
    setHeader("Date", getHttpDate());

    std::ostringstream oss;
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    if (!_body.empty() && _headers.find("Content-Length") == _headers.end())
    {
        _headers["Content-Length"] = sizeToString(_body.size());
    }
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end();
         ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }
    oss << "\r\n";
    oss << _body;

    return oss.str();
}