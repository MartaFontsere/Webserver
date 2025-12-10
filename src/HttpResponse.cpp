#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse()
    : _statusCode(200),
      _statusMessage("OK"),
      _httpVersion("HTTP/1.1")
{
}

HttpResponse::~HttpResponse()
{
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

    switch (code)
    {
    case 403:
        _statusCode = 403;
        _statusMessage = "Forbidden";
        _body = "<html><body><h1>403 Forbidden</h1></body></html>";
        break;

    case 404:
        _statusCode = 404;
        _statusMessage = "Not Found";
        _body = "<html><body><h1>404 Not Found</h1></body></html>";
        break;

    case 405:
        _statusCode = 405;
        _statusMessage = "Method Not Allowed";
        _body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        break;

    case 413:
        _statusCode = 413;
        _statusMessage = "Request Entity Too Large";
        _body = "<html><body><h1>413 Request Entity Too Large</h1>"
                "<p>Maximum body size is 10MB</p></body></html>";
        break;

    case 500:
    default:
        _statusCode = 500;
        _statusMessage = "Internal Server Error";
        _body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
        break;
    }

    // Headers necesarios mínimos
    _headers["Content-Type"] = "text/html";

    std::ostringstream length;
    length << _body.size();
    _headers["Content-Length"] = length.str();
}

std::string HttpResponse::buildResponse() const
{
    std::ostringstream oss;

    // Status line
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    // Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end();
         ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }

    // Mandatory blank line
    oss << "\r\n";

    // Body
    oss << _body;

    return oss.str();
}

/*
Contexto: qué debe tener una respuesta HTTP

    Una respuesta HTTP siempre sigue este formato:
        <HTTP-VERSION> <STATUS-CODE> <STATUS-MESSAGE>\r\n
        Header-Name: value\r\n
        Header-Name: value\r\n
        ...\r\n
        \r\n
        <body>

Explicación detallada de buildResponse()

std::string HttpResponse::buildResponse() const
{
    std::ostringstream oss;

std::ostringstream oss;
    Es un stream en memoria que te permite ir escribiendo texto como si fuera un std::cout, pero acabará convertido en un std::string.

    Es la forma más limpia (y compatible con C++98) de crear strings grandes concatenando muchas partes.

Status Line
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    Ejemplo que puede producir:
        HTTP/1.1 200 OK\r\n

Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
     it != _headers.end();
     ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }

    Recorres todos los headers almacenados en _headers, que es:
        std::map<std::string, std::string>

    Cada header se escribe del tipo:
        Content-Type: text/html\r\n
        Content-Length: 42\r\n

    Importante:
        Usas un map → los headers siempre salen en orden alfabético (es normal).

Línea en blanco obligatoria
    oss << "\r\n";

    Esta línea separa los headers del body.

    En HTTP, una línea vacía (CR LF) es la que indica:

    ➡️ “Ya he terminado de listar headers; lo que viene ahora es el cuerpo”

    Si no la pones, el navegador NO SABE dónde empieza el body, y puede interpretar que el body es otro header → respuesta inválida.

Body
    oss << _body;

    Aquí metes:
        HTML
        JSON
        texto
        lo que corresponda según el tipo de contenido

El body NO lleva \r\n obligatorio al final.
Solo se escribe tal cual.

Convertir todo a string
    return oss.str();

    Esto transforma todo lo que concatenaste en el ostringstream en un único std::string que luego enviarás al socket.

    Este string es exactamente el que _writeBuffer luego enviará al socket.
*/

/*
¿Qué cosas faltan o puedes mejorar más adelante?

Más adelante necesitarás:

    🔹 Ordenar headers importantes (opcional, pero recomendado)

        Content-Length

        Connection

        Date

    🔹 Incluir header Connection: keep-alive cuando corresponda

        En HTTP/1.1 se supone keep-alive por defecto, pero algunos navegadores lo requieren explícitamente.

    🔹 Añadir Date: obligatorio según RFC

        Ejemplo:
            Date: Tue, 15 Nov 1994 08:12:31 GMT

    🔹 Añadir header Server: webserv/1.0
    🔹 Añadir soporte para:
        respuestas sin body (204, 304)
        chunked encoding
        content-type dinámico según archivo
*/