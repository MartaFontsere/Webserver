#include "Client.hpp"
#include <arpa/inet.h> // inet_ntoa()
#include <iostream>    // para imprimir mensajes

/*
¿Por qué necesitamos Client.cpp?

Cuando tu servidor recibe una conexión (accept()), obtiene un nuevo file descriptor (FD) que representa a ese cliente específico.
Pero el servidor puede tener muchos clientes conectados al mismo tiempo.
→ Por tanto, necesitamos una forma clara de guardar y gestionar la información de cada cliente: su FD, su estado (si está leyendo o escribiendo), lo que ha enviado, lo que hay que responderle, etc.

La clase Client sirve justo para eso: encapsula todo lo que pasa con un cliente concreto dentro de un objeto.
Así evitamos caos y código duplicado dentro del servidor.
*/

Client::Client(int fd, const sockaddr_in &addr) : _clientFd(fd), _addr(addr), _closed(false), _headersComplete(false), _requestComplete(false), _contentLength(0)
{
}

Client::~Client()
{
    if (!_closed)
        close(_clientFd);
}

/*
Qué hacen:

Cuando el servidor acepta una nueva conexión (accept()), obtiene un clientFd (un nuevo socket) y una dirección, y crea un objeto Client que guarda:

    el descriptor de socket del cliente (fd),

    la struct contiene la dirección IP del cliente y el puerto del cliente (addr),

    y marca que la conexión no está cerrada (_closed = false).

El destructor se encarga de cerrar el socket cuando el cliente ya no se usa, evitando fugas de recursos.

➤ Por qué es necesario:

Cada cliente tiene su propio descriptor, y si no lo cierras correctamente cuando termina, el servidor se llenaría de conexiones abiertas y acabaría petando.
El destructor garantiza limpieza automática.
*/

int Client::getFd() const
{
    return (_clientFd);
}

/*
➤ Qué hace:

Simplemente devuelve el descriptor del cliente (para usarlo en poll(), select() o donde haga falta).

➤ Por qué lo necesitas:

El servidor debe poder vigilar la actividad de cada cliente en su bucle principal.
Gracias a este método, puede hacerlo sin exponer directamente los miembros internos del objeto.
*/

std::string Client::getIp() const
{
    return inet_ntoa(_addr.sin_addr);
}

/*
_addr.sin_addr → es un campo de tipo struct in_addr dentro de la estructura sockaddr_in.
Contiene la IP en formato binario (4 bytes para IPv4).

inet_ntoa() → convierte esa IP binaria en texto legible (por ejemplo, "192.168.0.25").

“ntoa” significa Network to ASCII.

El valor que devuelve inet_ntoa() es un char *, así que el constructor de std::string lo convierte automáticamente a std::string.
*/

//---------------- OPCION 2, POR SI ACASO --------------------
/*
std::string Client::getIp() const
{
    char buff[INET_ADDRSTRLEN]; // espacio para "xxx.xxx.xxx.xxx\0"
    // inet_ntop convierte la dirección binaria (sin_addr) a texto en formato IPv4.
    // Devuelve nullptr en error, o apunta a 'buff' en éxito.
    const char *res = inet_ntop(AF_INET, &_addr.sin_addr, buff, sizeof(buff));
    if (res == nullptr)
    {
        // en caso de error devolvemos cadena vacía (podrías devolver "0.0.0.0" o similar)
        return std::string();
    }
    return std::string(buff); // construye std::string desde C-string
}
*/
/*
Explicación línea a línea:

    char buf[INET_ADDRSTRLEN];
    Reserva un buffer en stack suficientemente grande para la representación textual de una IPv4 ("255.255.255.255" + \0).

    inet_ntop(AF_INET, &_addr.sin_addr, buf, sizeof(buf));

    AF_INET indica IPv4.

    &_addr.sin_addr es la dirección en formato binario (un in_addr).

    buf y sizeof(buf) dicen dónde escribir la cadena resultante.

    inet_ntop devuelve nullptr si falla (p. ej. tamaño insuficiente), o buf si tiene éxito.

    if (res == nullptr) return std::string();
    Manejo simple de error: devolver string vacío.

    return std::string(buf);
    Convierte el C-string buf en std::string y lo devuelve.

Por qué inet_ntop y no inet_ntoa:
    inet_ntoa() devuelve un puntero a una zona estática interna — no es thread-safe y su resultado se sobrescribe con cada llamada. inet_ntop() escribe en tu buffer y es segura y soporta IPv6 también (con AF_INET6).
    */

// FIN OPCION 2

bool Client::readRequest()
{
    char buffer[1024];
    int bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        if (bytesRead == 0)
            std::cout << "Cliente cerró la conexión\n";
        _closed = true;
        return false;
    }
    _request.append(buffer, bytesRead);

    // Si aún no hemos terminado de leer las cabeceras
    if (!_headersComplete)
    {
        if (!parseHeaders())
            return false; // aún no se han recibido todos los headers
    }

    // Si hay cuerpo, lo gestionamos aparte
    if (_headersComplete && _contentLength > 0)
    {
        if (!parseBody())
            return false; // aún no ha llegado todo el body
    }

    // Si llegamos aquí, ya tenemos toda la petición completa
    _requestComplete = true;
    return true;
}

/*
readRequest() lee bytes y acumula.

➤ Qué hace:
Creamos un buffer temporal donde guardaremos los bytes que recibimos.
Tamaño 1024 bytes (1 KB).

Llamamos a recv() para leer datos del socket del cliente.
    _fd → socket del cliente.
    buffer → dónde guardar los datos.
    sizeof(buffer) .
    0 → flags (aquí no usamos ninguno especial).

👉 Si el cliente ha cerrado la conexión o hay error, recv() devuelve 0 o -1.

⚠️ if (bytesRead <= 0)
Si no hay datos o error:
    Marcamos _closed = true
    Y hacemos return (dejamos de procesar).

_request += buffer; o   _request.append(buffer, bytesRead);
    Guardamos los datos leídos en la petición completa del cliente (_request es un std::string).
    Así podemos recibir datos por partes si la petición llega fragmentada.


Una petición HTTP puede contener cabecera (headers) y cuerpo (body) —y deberías controlar ambas, al menos mínimamente, si quieres un servidor correcto o extensible.

Estructura general de una petición HTTP:
    Una petición (por ejemplo, de un navegador al servidor) tiene esta forma:
        GET /index.html HTTP/1.1\r\n
        Host: localhost:8080\r\n
        User-Agent: curl/7.68.0\r\n
        Accept: ...\r\n
        \r\n

    O si envía datos (por ejemplo, un POST):
        POST /api/data HTTP/1.1\r\n
        Host: localhost:8080\r\n
        Content-Type: application/json\r\n
        Content-Length: 27\r\n
        \r\n
        {"nombre": "Marta", "edad": 25}


Partes principales
    🔸 a) Request line
    La primera línea:
        GET /index.html HTTP/1.1

    Contiene:
        Método (GET, POST, PUT, DELETE, etc.)
        Ruta (/index.html)
        Versión del protocolo (HTTP/1.1)

    🔸 b) Headers (cabeceras)
    Son pares clave-valor:
        Content-Type: application/json
        Content-Length: 27

    Indican metadatos de la petición: tipo de contenido, tamaño, conexión persistente, cookies, etc.

    🔸 c) Body (cuerpo)
    Solo aparece en métodos que envían datos, como POST, PUT, o PATCH.
    Contiene el contenido real (texto, JSON, binario, formulario...).

Para saber si hay cuerpo debes mirar si existe
    El header: Content-Length: N
    o a veces: Transfer-Encoding: chunked
        Si hay Content-Length, el body tiene exactamente N bytes.

        Si hay Transfer-Encoding: chunked, el cuerpo llega en fragmentos codificados (más avanzado, puedes ignorarlo de momento).

        Si no hay ninguno, normalmente no hay cuerpo (como en la mayoría de GET).

Para tu servidor actual (básico), lo ideal sería:
    Leer hasta \r\n\r\n → eso marca el final de las cabeceras.
    Parsear las cabeceras → guarda si existe Content-Length.
    Si Content-Length > 0, espera leer exactamente esos bytes más para tener el body completo.
    Cuando tengas todo (headers + body) → procesas la petición.



➤ Cosas clave:

    recv() es como read(), pero específico para sockets.

    bytesRead > 0
        → Recibiste algunos bytes (aunque sea menos de 1024).
        → No significa que se haya terminado; solo que por ahora eso es lo que llegó.
        → Los añades a _request y sigues.
        → La próxima vez que poll() diga que hay más datos, vuelves a llamar a readRequest().

    bytesRead == 0 significa que el cliente se desconectó o ya no llegan mas datos
    bytesRead < 0 significa que hubo error.
        → Puede ser error temporal (EAGAIN si el socket es no bloqueante), o real.
        → Si es EAGAIN, simplemente no había datos en ese momento, y ya volverás a leer más tarde.

SEGUIMOS CON EL CÓDIGO:
if (!_headersComplete)
    {
        if (!parseHeaders())
            return false; // aún no se han recibido todos los headers
    }

    headersComplete -> Es una bandera de estado (bool) dentro de la clase Client
        false → todavía estamos recibiendo la cabecera (headers HTTP).
        true → ya hemos recibido el final de los headers (\r\n\r\n).
    Este valor se guarda y recuerda entre llamadas a readRequest().
    Así, si la cabecera ya se procesó, no la volvemos a analizar cada vez que llegan más datos.

    parseHeaders() no solo marca el final, sino que extrae información útil (como Content-Length, Host, Content-Type, etc.) y prepara el siguiente paso.

if (_headersComplete && _contentLength > 0)
    {
        if (!parseBody())
            return false; // aún no ha llegado todo el body
    }

    Una vez sabemos que ya esta el header completo, y en el caso de que se haya encontrado un content lenght, entonces parseamos el body. En este parseo miramos si está todo. en caso de que esté lo guardamos.

*/

bool Client::parseHeaders()
{
    // 🔍 Comprobamos si la petición HTTP está completa
    // Buscamos el final de la cabecera (header) HTTP, que termina con "\r\n\r\n"
    size_t headerEnd = _request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) // significa “no encontrado” o “posición inválida”
        return false;                   // aún no ha llegado toda la cabecera

    _headersComplete = true; // Si llega hasta ahquí, significa que ha encontrado el final

    // Extraemos y guardamos solo la parte de la cabecera
    std::string headerPart = _request.substr(0, headerEnd);

    // Buscamos si hay un Content-Length
    size_t contentLengthPos = headerPart.find("Content-Length:");
    if (contentLengthPos != std::string::npos)
    {
        // Leemos el número tras "Content-Length:"
        size_t endLine = headerPart.find("\r\n", contentLengthPos);
        std::string value = headerPart.substr(contentLengthPos + 15, endLine - (contentLengthPos + 15));
        _contentLength = std::atoi(value.c_str());
    }
    else
        _contentLength = 0;

    return true;
    // SOLO SE CONTEMPLA CONTENT LENGHT, FALTA CHUNKS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

/*
parseHeaders() detecta si llegaron las cabeceras completas (\r\n\r\n).

Si encuentra el delimitador \r\n\r\n, significa que la cabecera HTTP está completa (ya se ha recibido la petición entera)

➤ Por qué es necesario:
    Las peticiones HTTP no siempre llegan de una sola vez.
    Un cliente puede enviar una parte ahora y otra dentro de unos milisegundos.
    Este método permite leer de forma incremental hasta tener la petición completa. Como el cliente puede seguir enviando fragmentos parciales, si no encontramos la secuencia final, devolveremos false.

Si encontramos el final del header:
    Primero guardamos la parte de la cabecera
    Buscamos si dentro del header hay Content-Length para saber si hay body.
        Si lo hay:
            Esto indica que la petición tiene un cuerpo (por ejemplo, un POST o PUT).
            Leemos el valor numérico y comprobamos si el tamaño actual del buffer ya contiene cabecera + body completo.
            Entonces:
                size_t endLine = headerPart.find("\r\n", contentLengthPos); -> Busca el final de esa línea (\r\n)

                std::string value = headerPart.substr(contentLengthPos + 15, endLine - (contentLengthPos + 15)); -> Corta el trozo de texto que está entre "Content-Length:"(por eso el +15 caracteres) y el salto de línea → o sea, el número.
                    find() apunta a la posicion de inicio de lo que buscas, por eso cuando queremos encontrar el numero hay que sumar 15 caracteres, que son los que tiene exactamente Content-Length

                _contentLength = std::atoi(value.c_str()); -> Convierte ese número en entero (std::atoi) y lo guarda en _contentLength.

        Si no hay cabecera "Content-Length:", asumimos que no hay cuerpo (body), por lo tanto _contentLenght será 0
*/

bool Client::parseBody()
{
    // Localizamos el inicio del body: justo después de "\r\n\r\n"
    size_t bodyStart = _request.find("\r\n\r\n") + 4;

    // Si no tenemos todavía todos los bytes del body, volvemos al bucle y esperamos a la siguiente vuelta
    size_t bodyBytes = _request.size() - bodyStart;
    if (bodyBytes < static_cast<size_t>(_contentLength))
        return false;

    // Si llegamos aquí, ya tenemos todo
    // Guardamos el cuerpo completo
    _body = _request.substr(bodyStart, _contentLength);

    return true;
}

/*
size_t bodyStart = _request.find("\r\n\r\n") + 4;
    _request contiene toda la petición recibida hasta ahora, incluyendo headers y body.
    find("\r\n\r\n") devuelve la posición del primer \r\n\r\n, es decir, el final de los headers.
    +4 → avanzamos justo después del \r\n\r\n, que es donde empieza el body.

size_t bodyBytes = _request.size() - bodyStart;
    Calculamos cuántos bytes de body ya hemos recibido.
    _request.size() → total de datos que tenemos
    bodyStart → posición donde empieza el body
    bodyBytes = cantidad de bytes del body que ya llegaron.

if (bodyBytes < static_cast<size_t>(_contentLength))
    return false;

    _contentLength → lo que el cliente dijo que iba a enviar en la cabecera Content-Length.
    Si aún no tenemos todos los bytes del body, devolvemos false.
    Esto indica al servidor: “no he terminado de leer la petición; vuelve a llamar cuando llegue más data”.

    📌 Aquí no hacemos bucles: la función solo revisa si ya está todo, y si no, se sale.

*/

bool Client::sendResponse(const std::string &msg)
{
    if (send(_clientFd, msg.c_str(), msg.size(), 0) < 0)
    {
        _closed = true; // Marcamos al cliente como cerrado para que el servidor deje de usarlo
        return false;
    }
    return true;
}

/*
➤ Qué hace:
    Llama a send() para escribir el mensaje en el socket del cliente
    Devuelve true si todo fue bien, false si hubo error (socket cerrado o fallo del sistema).

➤ Por qué es necesario:
    Después de leer la petición, hay que responder.
    Y aunque ahora la respuesta sea fija, en el futuro podrías analizar _request y construir una respuesta personalizada.

➤ Cosas clave:
    send() es la versión de write() para sockets.
    La cabecera Content-Length debe coincidir con el tamaño del cuerpo (13 en “Hello, world!”).
    Si quisieras mandar más datos, podrías fragmentarlos y seguir mandando.
*/

bool Client::isClosed() const
{
    return _closed;
}

/*
Comprueba si la conexión con este cliente ya se ha cerrado (por error o desconexión).
Se usa para que el servidor sepa si debe eliminar este cliente de la lista activa o no seguir intentando leer/escribir.
*/