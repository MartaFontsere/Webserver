#pragma once

#include <map>
#include <string>

class HttpResponse {
private:
  int _statusCode;
  std::string _statusMessage;
  std::string _httpVersion;

  std::map<std::string, std::string> _headers;
  std::string _body;
  bool _cgiPending; // True when CGI async is in progress

public:
  HttpResponse();
  ~HttpResponse();

  // setters
  void setStatus(int code, const std::string &message);
  void setHeader(const std::string &key, const std::string &value);
  void setBody(const std::string &body);
  int getStatusCode() const;

  // CGI async state
  void setCGIPending(bool pending);
  bool isCGIPending() const;

  // ensamblar respuesta final
  std::string buildResponse() const; // → devuelve el string final

  // helpers para errores
  void setErrorResponse(int code);

  // método para obtener mensaje de status
  static std::string getHttpStatusMessage(int code);
};

/*
HttpResponse
    Genera cabecera + cuerpo.
    NO interpreta la request por sí solo.
    NO sabe nada de sockets.

Lo que se queda en Client
    ✔ seleccionar qué status poner
    ✔ asignar headers necesarios
    ✔ asignar body
    ✔ llamar a buildResponse()
    ✔ enviar con send()

Lo que se va a HttpResponse
    ✔ ensamblar el string final
    ✔ ordenar headers
    ✔ construir la status line
    ✔ añadir CRLF correctamente


¿Qué es una respuesta HTTP?

Cuando el cliente (navegador, curl, etc.) envía una request, tu servidor debe
devolverle una response siguiendo el formato del protocolo HTTP/1.1.

Una respuesta HTTP siempre tiene tres partes:
    Status line (línea de estado)

    Headers

    Body (opcional, depende del tipo de respuesta)

Ejemplo típico:
    HTTP/1.1 200 OK
    Content-Type: text/plain
    Content-Length: 12

    Hello world!


🧩 1) STATUS LINE (Línea de estado)

Formato:
    HTTP/<version> <status code> <reason phrase>

Ejemplos:
    HTTP/1.1 200 OK

    HTTP/1.1 404 Not Found

    HTTP/1.1 500 Internal Server Error

En tu servidor, tú construyes esa línea según lo que quieras responder.
Para empezar, 200 OK te vale casi siempre.


🧩 2) HEADERS

Son líneas extra que dan metadatos sobre la respuesta.

Formato general:
    Header-Name: valor

Ejemplos comunes:
| Header             | Función                                           |
| ------------------ | ------------------------------------------------- |
| **Content-Length** | Tamaño exacto del body en bytes                   |
| **Content-Type**   | Tipo del contenido (text/html, application/json…) |
| **Connection**     | keep-alive o close                                |
| **Date**           | Fecha del servidor                                |
| **Server**         | Nombre del software del server                    |


En tu caso, estás generando esto:
    Content-Type: text/plain
    Content-Length: 12

Esto significa:
    El body es texto normal.
    Tiene exactamente 12 bytes.

Tu webserver debe ser estricto con Content-Length, porque los navegadores lo
usan para saber cuándo termina la respuesta.


🧩 3) BODY (opcional)
Es el contenido real que ve el usuario.

Ejemplo:
    Hello world!


Puede ser:
    HTML
    JSON
    Binarios (imágenes, PDF…)
    Texto plano
    Lo que sea

El body empieza justo después de un salto doble:
    \r\n\r\n


Ejemplo completo:
    HTTP/1.1 200 OK\r\n
    Content-Type: text/plain\r\n
    Content-Length: 12\r\n
    \r\n
    Hello world!


🔥 ¿Cómo lo usa tu servidor?

Ahora mismo haces:

std::string response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 12\r\n"
    "\r\n"
    "Hello world!";


Esto es correctísimo.

Después haces:
    client->sendResponse(response);


Y cuando el cliente recibe esto, el navegador sabe que:
    La conexión es válida
    El body tiene 12 bytes
    El contenido es texto
    Después de esos 12 bytes ya ha terminado

🧠 Cosas importantes que tendrás que añadir más adelante
1️⃣ Manejar distintos códigos de estado
    404, 400, 500, 201…

2️⃣ Content-Type según la extensión
Si sirves archivos:
    .html → text/html
    .css → text/css
    .jpg → image/jpeg

3️⃣ Soportar “Connection: keep-alive”
    HTTP/1.1 asume keep-alive por defecto.

4️⃣ Enviar archivos grandes en múltiples chunks
    Solo si quieres usar chunked encoding.

*/