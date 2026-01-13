#include "http/HttpResponse.hpp"
#include <ctime>
#include <sstream>

// ==================== CONSTRUCTORS ====================

HttpResponse::HttpResponse()
    : _statusCode(200), _statusMessage("OK"), _httpVersion("HTTP/1.1"),
      _cgiPending(false) {}

HttpResponse::~HttpResponse() {}

// ==================== STATIC HELPERS ====================

/**
 * @brief Generates current date-time in HTTP format (RFC 9110)
 * Format: "Day, DD Mon YYYY HH:MM:SS GMT"
 */
static std::string getHttpDate() {
  time_t currentTime;
  time(&currentTime);
  struct tm *timeInfo = gmtime(&currentTime);
  char buffer[80];
  strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
  return std::string(buffer);
}
/*
Esto es el header HTTP Date: que indica cuándo el servidor generó la respuesta.
Usa gmtime (UTC/GMT) porque el RFC lo exige. El header Date: es obligatorio
según el RFC de HTTP.
*/

/**
 * @brief Maps HTTP status code to standard reason phrase
 * @note Public static method - usable from other modules (e.g., CGIHandler)
 */
std::string HttpResponse::getHttpStatusMessage(int code) {
  switch (code) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Request Entity Too Large";
  case 500:
  default:
    return "Internal Server Error";
  }
}

// ==================== SETTERS ====================

void HttpResponse::setStatus(int code, const std::string &message) {
  _statusCode = code;
  _statusMessage = message;
}

void HttpResponse::setHeader(const std::string &key, const std::string &value) {
  _headers[key] = value;
}

void HttpResponse::setBody(const std::string &body) {
  _body = body;
  std::ostringstream oss;
  oss << _body.size();
  _headers["Content-Length"] = oss.str();
}

int HttpResponse::getStatusCode() const { return _statusCode; }

void HttpResponse::setCGIPending(bool pending) { _cgiPending = pending; }

bool HttpResponse::isCGIPending() const { return _cgiPending; }

// ==================== ERROR HANDLING ====================

void HttpResponse::setErrorResponse(int code) {
  _httpVersion = "HTTP/1.1";
  _statusCode = code;
  _statusMessage = getHttpStatusMessage(code);

  switch (code) {
  case 400:
    _body = "<html><body><h1>400 Bad Request</h1></body></html>";
    break;
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
    break;
  }

  _headers["Content-Type"] = "text/html";
  std::ostringstream length;
  length << _body.size();
  _headers["Content-Length"] = length.str();
}

// ==================== RESPONSE BUILDER ====================

std::string HttpResponse::buildResponse() const {
  std::ostringstream oss;

  // Status line
  oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

  // Automatic headers (RFC-compliant)
  oss << "Server: webserv/1.0\r\n";
  oss << "Date: " << getHttpDate() << "\r\n";

  // User-set headers
  for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
       it != _headers.end(); ++it) {
    oss << it->first << ": " << it->second << "\r\n";
  }

  // Mandatory blank line + body
  oss << "\r\n";
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
    Es un stream en memoria que te permite ir escribiendo texto como si fuera un
std::cout, pero acabará convertido en un std::string.

    Es la forma más limpia (y compatible con C++98) de crear strings grandes
concatenando muchas partes.

Status Line
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage <<
"\r\n";

    Ejemplo que puede producir:
        HTTP/1.1 200 OK\r\n

Headers
    for (std::map<std::string, std::string>::const_iterator it =
_headers.begin(); it != _headers.end();
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

    Si no la pones, el navegador NO SABE dónde empieza el body, y puede
interpretar que el body es otro header → respuesta inválida.

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

    Esto transforma todo lo que concatenaste en el ostringstream en un único
std::string que luego enviarás al socket.

    Este string es exactamente el que _writeBuffer luego enviará al socket.
*/

/*
Estado actual de HttpResponse (8.1.26):

  ✅ Content-Length: calculado automáticamente en setBody()
  ✅ Date: añadido automáticamente en buildResponse() (RFC 9110)
  ✅ Server: añadido automáticamente en buildResponse()
  ✅ Connection: keep-alive lo gestiona RequestHandler::_applyConnectionHeader()
  ✅ Content-Type dinámico: lo gestiona StaticFileHandler (donde pertenece)
  ✅ getHttpStatusMessage(): helper público para traducir códigos a mensajes

Posibles mejoras futuras (no críticas para el proyecto):
  🔹 Chunked encoding (Transfer-Encoding: chunked) para streaming
  🔹 Validación especial para 204/304 (no deberían tener body)
*/