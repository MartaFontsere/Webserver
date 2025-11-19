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

/*
Objetivo: que Client solo lea bytes del socket y no se encargue de entender HTTP.
La lógica de parseHeaders, Content-Length, etc. se moverá a una clase HttpRequest.
*/

Client::Client(int fd, const sockaddr_in &addr) : _clientFd(fd), _addr(addr), _closed(false), _writeBuffer(), _writeOffset(0), _lastActivity(time(NULL)), _requestComplete(false)
{
}

/*
 _contentLength(0) podría interpretarse como “esperamos 0 bytes de body” — mejor usar -1 para decir “no hay Content-Length definido”.
*/

Client::~Client()
{
    if (!_closed)
    {
        std::cout << "[Client] Cerrando conexión con " << getIp() << std::endl;
        if (_clientFd != -1)
            close(_clientFd);
        _closed = true;
        _clientFd = -1;
    }
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

Explicación:
    if (!_closed) → Evita que se cierre dos veces.

    if (_clientFd != -1) → Asegura que no se llame a close() con un fd inválido (aunque no crashearía, es más limpio).

    close(_clientFd) → Cierra el socket si aún está abierto.

    _closed = true; → Marca el objeto como cerrado para evitar dobles acciones.

    _clientFd = -1; → Evita que este número se use accidentalmente si el fd se reutiliza en el sistema.

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
    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &_addr.sin_addr, ipStr, sizeof(ipStr)) != NULL)
        return std::string(ipStr);
    return "Unknown IP";
}

/*

 return inet_ntoa(_addr.sin_addr);

OBSOLETO
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
            std::cout << "[Info] Cliente (fd: " << _clientFd << ") cerró la conexión normalmente.\n";
        else
            std::cerr << "[Error] Fallo al leer del cliente con recv() (fd: " << _clientFd << "): " << strerror(errno) << "\n";
        _closed = true;
        return false;
    }
    std::cout << "\nEmpezando a leer la Request (fd: " << _clientFd << ").\n";
    _rawRequest.append(buffer, bytesRead);

    std::cout << "  # Request recibida (fd: " << _clientFd << "):\n"
              << _rawRequest;

    _lastActivity = time(NULL);

    // Intentamos parsear la request actual
    std::cout << "[Debug] Parseando request del cliente fd " << _clientFd << std::endl;
    if (_httpRequest.parse(_rawRequest))
    {
        std::cout << "✅ Request completa (client fd: " << _clientFd << ")\n";
        _requestComplete = true;
        _rawRequest.clear(); // 👈 limpiamos el buffer raw porque la información queda almacenada en _httpRequest, asi rawRequest queda limpio para la próxima request

        // IMPORTANTEEEEEEE!!!!
        // Nota: más adelante, si quieres soportar pipelining, cambia esto por _rawRequest.erase(0, parsedBytes) y haz que HttpRequest devuelva parsedBytes.
    }

    // ⚙️ En cualquier caso (falta data o ya completa),
    // seguimos vivos y listos para la siguiente vuelta
    return true;
}

/*
7.11.25
¿Hay que crear el objeto httpRequeest en readRequest()?

❌ No hace falta crearlo explícitamente con new ni llamando a un constructor.
El objeto _httpRequest se crea automáticamente cuando se construye el Client, igual que cualquier otro miembro.

Simplemente, cuando leas del socket en readRequest(), irás acumulando los datos en _requestBuffer, y cuando veas que está completa, llamarás a:
    _httpRequest.parse(_requestBuffer);



resumen del flujo completo
    1️⃣ poll() detecta POLLIN.
    2️⃣ handleClientEvent() llama a readRequest().
    3️⃣
        Si readRequest() devuelve false → cliente cerrado.

        Si devuelve true pero isRequestComplete() es false → esperar más data.

        Si devuelve true y isRequestComplete() es true → enviar respuesta.
        4️⃣ Si falta enviar algo, se activa POLLOUT.
        5️⃣ Cuando el kernel indique POLLOUT, el bucle principal llama flushWrite().
        6️⃣ Cuando ya no haya nada pendiente, se desactiva POLLOUT.
        */

/*
4.11.25

Ahora mismo tu flujo es así:

recv() → procesas → send() → cliente cierra → servidor marca _closed → cleanup lo borra


✅ Funciona, pero es HTTP/1.0 style: cada petición = nueva conexión.

En HTTP/1.1, por defecto las conexiones son persistentes (keep-alive),
lo que significa que el cliente puede mandar varias peticiones seguidas por el mismo socket sin cerrarlo.

Ahora, cuando ya no hay bytes que mandar (bytes = 0)

Qué significa bytesRead == 0
        Cuando recv() devuelve 0, no es que haya terminado de mandar la petición, sino que el cliente ha cerrado su socket TCP.
        👉 Es decir: ya no hay canal abierto para seguir comunicándose.

        Esto pasa típicamente en dos casos:
            Cliente desconecta (por ejemplo, el navegador cierra la pestaña o la conexión HTTP/1.0 no mantiene keep-alive).

            Cliente ha terminado la comunicación y cierra el socket voluntariamente.

        Por tanto, sí:
            Cuando bytesRead == 0, hay que marcar el cliente como cerrado (_closed = true), porque el socket ya no sirve para nada más.

        ¿por qué parece que “cerramos siempre todos”?
            Porque en el flujo actual estás probablemente haciendo HTTP/1.0 o HTTP/1.1 sin keep-alive, y en ambos casos el cliente cierra la conexión tras cada petición (salvo que explícitamente indique Connection: keep-alive).
            Así que el servidor recibe la petición → responde → el cliente cierra → bytesRead == 0 → cerramos el cliente.

            👉 En ese flujo es normal que se cierren “todos”, uno por petición.

            Y si quiero mantener viva la conexión?
                Para que no se cierre el cliente después de cada petición, hay que comprobar si el cliente quiere mantener la conexión viva.
                Eso se indica en la cabecera:

                Connection: keep-alive

                Entonces podrías cambiar el comportamiento de readRequest()


Entonces tu servidor debería:
    Detectar si el cliente quiere mantener la conexión viva.
    No marcar _closed = true en ese caso.
    Esperar más datos en el mismo poll().

*/

/*
3.11.25

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

// HELPPER
void Client::applyConnectionHeader()
{
    if (_httpRequest.isKeepAlive())
        _httpResponse.setHeader("Connection", "keep-alive");
    else
        _httpResponse.setHeader("Connection", "close");
}

bool Client::processRequest()
{
    // 1) Reseteamos cualquier HttpResponse previa (estado limpio) -> Limpia HttpResponse previo
    _httpResponse = HttpResponse(); // crea un HttpResponse por defecto y lo copia en el miembro //AQUÍ O AL ACABAR DE USARLO LO DEJAMOS LISTO PARA LA PRÓXIMA??? LA PRIMERA VEZ QUE SE USE YA SE CREA SOLO CON CLIENT, ASI QUE NO PASARÍA NADA

    // 2) Validar método HTTP (por ahora solo admitimos GET)
    const std::string &method = _httpRequest.getMethod();
    if (method != "GET")
    {
        // Respondemos con 405 Method Not Allowed (contenido + headers dentro de HttpResponse)
        _httpResponse.setErrorResponse(405);
        applyConnectionHeader(); // Así el servidor maneja keep alive corretamente tanto en rutas validas como en invalidas (erroes), siempre coherente
        return true;             // hemos generado una respuesta válida -> no es un fallo fatal
    }

    // 2. Validar ruta
    const std::string &path = _httpRequest.getPath();

    if (path != "/") // caso para empezar
    {
        _httpResponse.setErrorResponse(404);
        applyConnectionHeader();
        return true;
    }

    // 3. Todo OK → generar respuesta 200
    std::string body = "<html><body><h1>Hello World!</h1></body></html>";

    _httpResponse.setStatus(200, "OK");
    _httpResponse.setHeader("Content-Type", "text/html");
    _httpResponse.setHeader("Content-Length", std::to_string(body.size()));
    applyConnectionHeader();
    _httpResponse.setBody(body);

    std::cout << "\nProcess Request (fd=" << _clientFd << "):\n  method = GET\n  status = 200)\n";
    return true;
}

/*
✔ Client decide qué respuesta toca, concretamente aquí decides:
    método
    ruta
    errores 404 / 405 / 500
    generar el body

✔ Client decide contenido
✔ HttpResponse decide formato

processRequest devuelve:
    True: ha ido todo bien, hemos generado una respuesta
    False: error grave, cerrar cliente
        Ejemplo:
            Fallo al abrir un archivo del disco
            Fallo al ejecutar un CGI
            Fallo lígico interno / nenirt error
            Corrupción de la request
            ...
        De momento no tengo errores que requieran cerrar el cliente PERO TENER EN CUENTA PARA EL FUTURO, COSAS A AÑADIR!!!!!!!!!!!!!!!!!!!


Explicación del código:

if (!_requestComplete) return true;
    Qué hace: comprueba si Client::readRequest() ya marcó que la petición está completa. Si no, vuelve sin tocar nada.

    Por qué: no tiene sentido generar una respuesta si todavía faltan bytes (por ejemplo, headers incompletos o body no recibido).

    Por qué return true: esto no es error: simplemente indica “no procesado aún — seguir esperando”. En el Server::handleClientEvent ese true significa que no hay fallo fatal y que el Client queda activo.

_httpResponse = HttpResponse();
    Qué hace: asigna un HttpResponse nuevo por valor al miembro _httpResponse. Es una forma rápida de “resetear” cualquier contenido/headers anteriores.

    Por qué: evitamos mezclar una respuesta antigua con la nueva; limpiamos el estado antes de construirla.

    Nota de implementación: esto usa el operador de asignación. Si HttpResponse tiene un buen constructor por defecto y no gestiona recursos raros, está bien. Alternativa: _httpResponse.reset() si implementas un método reset() dentro de HttpResponse.

    VALORAR SI HACER UNA FUNCIÓN RESET PARA EL FINAL DE CUANDO SE HA ACABADO CON HTTPRESPONSE, SI ES MAS PROFESIONAL O ASI YA VA BIEN

const std::string &method = _httpRequest.getMethod();
    Qué hace: obtiene (por referencia constante) la cadena con el método ("GET", "POST", ...).

    Por qué usar const &: evita copiar la cadena (más eficiente) y previene modificaciones accidentales.

    Importante: HttpRequest::getMethod() debe devolver const std::string& para que no haya una copia temporal. Si devuelve por valor, seguiría funcionando pero habría copia.

if (method != "GET") { _httpResponse.setErrorResponse(405); return true; }
    Qué hace: valida que el método sea GET. Si no, prepara una respuesta 405 (Method Not Allowed).

    Por qué no cerramos la conexión ni devolvemos false: una petición con método no permitido no es un fallo interno del servidor, es una petición válida que se responde con un código HTTP. Por eso devolvemos true (hemos generado respuesta), y luego sendResponse() enviará la 405.

    Dónde se define setErrorResponse: en HttpResponse. Debe fijar _statusCode, _statusMessage, _body, y headers básicos como Content-Type y Content-Length.

const std::string &path = _httpRequest.getPath();
    Qué hace: obtiene la ruta solicitada (ej. "/", "/index.html").

    Por qué: a partir de la ruta decides si servir un archivo, redirigir, error 404, etc.

if (path != "/") { _httpResponse.setErrorResponse(404); return true; }
    Qué hace: ejemplo simple: aceptas solo /. Si no, preparas 404.

    Por qué: de nuevo, esto no es un fallo del servidor, es comportamiento esperado ante una ruta no encontrada → preparas una respuesta y devuelves true.

Construcción del body y headers 200:
    body: la respuesta que quieres enviar. Puede salir de un archivo, ser generada dinámicamente, lo que necesites.

    setStatus: fija el código y el mensaje de estado.

    setHeader("Content-Type", ...): informa al cliente cómo interpretar el body.

    Content-Length: número de bytes del body. Muy importante en HTTP/1.1 si no usas chunked. Aquí usamos std::to_string(body.size()) — claro y legible.

    setBody: guarda el body en el objeto HttpResponse para que buildResponse() lo inserte al final.


Notas importantes sobre diseño y flujo
    processRequest() no envía.
        Su responsabilidad es únicamente decidir qué respuesta deben enviar y construirla dentro de _httpResponse. El envío lo hace sendResponse() — separación de responsabilidades.

    ¿Qué devuelve true y false?
        true → procesamiento OK (aunque la respuesta sea 404/405).

        false → error fatal (por ejemplo, fallo interno irreparable, recursos agotados) y Server debería abandonar el cliente. En este diseño, las rutas no válidas o métodos no soportados no son false.

    Content-Length:
        Es obligatorio si no usas Transfer-Encoding: chunked.

        Cuando más adelante sirvas archivos, calcula filesize y pon Content-Length con std::to_string.

    Keep-Alive / Connection:
        Aquí no toques la conexión. Client::sendResponse() será quien decida, en función de headers de la request (Connection: close o keep-alive), si mantiene la conexión abierta y resetea estado para la siguiente petición.

    Escalabilidad:
        Cuando añadas más rutas, processRequest() puede delegar a un pequeño enrutador que busque handlers por path y method. Mantén processRequest() como punto de entrada.

 */

bool Client::sendResponse()
{
    std::string msg = _httpResponse.buildResponse();

    // 1. Encolar respuesta
    if (/*!_closed && */ !msg.empty()) // no tiene sentido mirar closed aqui, porque si lo estaba debido a un error que he comprobado ya habria salido antes, y si se desconecta el cliente por su lado despues del recv() lo vere en el send, antes de intentarenviar
        _writeBuffer.append(msg);

    // 2. Intentar enviar lo que haya
    if (!flushWrite())
        return false; // flushWrite() ya marcará closed = true si hubo error

    // 3. Si todavía hay bytes pendientes, el servidor deberá activar POLLOUT
    if (hasPendingWrite())
        return true; // está todo correcto, pero falta enviar, server activará POLLOUT

    // 4. Si no queda nada pendiente, todo enviado -> según keep-alive, marcar cerrado o dejar abierto
    if (!_httpRequest.isKeepAlive())
    {
        // cerrar la conexión limpiamente (marcar para que cleanup la borre)
        _closed = true;
        std::cout << "[Client] Respuesta completa. Cierre por Connection: close (fd: " << _clientFd << ")" << std::endl;
    }
    else
    {
        // mantener la conexión abierta para próximas peticiones
        // además limpiar buffers de request para la siguiente
        _httpRequest.reset(); // <-- limpia headers, body, etc.
        _requestComplete = false;
        std::cout << "[Client] Respuesta completa, manteniendo conexión (keep-alive fd: " << _clientFd << ")\n    Esperando nueva request" << std::endl;
    }

    return true;
}

/*
14.11.25
Aquí imprimimos lo que se ha decidido en process request

Explicación del código:

std::string msg = _httpResponse.buildResponse();
    Aquí generas un string completo de la respuesta HTTP:
        buildResponse() concatena:
            status line
            headers
            CRLF final
            body
    Aquí produces UN SOLO string final, listo para enviar.

El resto de código sigue igual
*/

/*
Principios sencillos antes de tocar código
    send() en sockets no-bloqueantes puede:
        devolver >0 (n bytes enviados),
        devolver 0 (peer cerró la conexión),
        devolver -1 con errno == EAGAIN/EWOULDBLOCK (no se puede escribir ahora; no es error grave),
        devolver -1 con otro errno (error grave).

    Por eso hay que acumular la respuesta en un buffer y reenviar hasta que esté todo escrito.

    POLLOUT es la notificación de poll() que te dice “esto ahora es escribible”; la activas cuando tienes datos pendientes y la desactivas cuando acabas.



Queremos mantener un método intuitivo como:

    bool Client::sendResponse(const std::string &msg);


Y que dentro se encargue de:
    añadir al buffer (_writeBuffer),
    intentar enviar (flushWrite()),
    decidir si marcar POLLOUT o si ya está todo enviado,
    marcar el cliente cerrado si no tiene keep-alive, etc.

Explicación de flujo (paso a paso):
    Se llama sendResponse() desde el servidor cuando ya tienes la respuesta generada.
    → Añade esa respuesta al buffer (_writeBuffer).

    Llama a flushWrite() para intentar enviarla inmediatamente.
    → Si la conexión está lista para escribir, se mandará parte o todo.
    → Si el socket está lleno (EAGAIN), no pasa nada: el resto queda en _writeBuffer.

    Comprueba si quedan bytes pendientes.
        Si sí → se devolverá true y el servidor sabrá que debe activar POLLOUT para seguir enviando.

        Si no → ya se ha enviado todo, limpia buffer y decide:
            Si no hay keep-alive, marcar _closed = true para que cleanupClosedClients() lo borre.

            Si hay keep-alive, mantener la conexión abierta.



Qué pasa con poll() y POLLOUT
    Esto lo entenderás mejor ahora que tienes clara la separación:
        El servidor principal tiene una lista de clientes y hace poll() sobre sus sockets.

        Cuando a un cliente se le activa el bit POLLOUT, eso significa:
            “El kernel te avisa que el socket puede aceptar más datos para escribir”.

    En ese momento, el servidor llama de nuevo a flushWrite() para continuar enviando lo que quedaba pendiente.

    Así que flushWrite() se usa tanto:
        Cuando tú generas la respuesta por primera vez (intento inicial).
        Como cuando el socket avisa que ya puede seguir escribiendo.

*/

bool Client::hasPendingWrite() const
{
    return (_writeOffset < _writeBuffer.size());
}

/*
indica si hay bytes pendientes por enviar en el cliente.

Server necesita saber si activar POLLOUT para un cliente; si hay datos pendientes, activa POLLOUT, si no, no.
*/

bool Client::flushWrite()
{
    if (_writeOffset >= _writeBuffer.size())
    {
        _writeBuffer.clear();
        _writeOffset = 0;
        std::cout << "[Info] Respuesta completa enviada al cliente (fd: " << _clientFd << ")\n";
        return true;
    }

    const char *buf = _writeBuffer.data() + _writeOffset;
    size_t remaining = _writeBuffer.size() - _writeOffset;

    ssize_t s = send(_clientFd, buf, remaining, 0);
    if (s > 0)
    {
        _writeOffset += static_cast<size_t>(s);
        _lastActivity = time(NULL);
        std::cout << "\n[Info] Enviando respuesta al cliente (fd: " << _clientFd << "):\n"
                  << buf << "\n\n";
        if (_writeOffset >= _writeBuffer.size())
        {
            _writeBuffer.clear();
            _writeOffset = 0;
            std::cout << "[Info] Respuesta completa enviada al cliente (fd: " << _clientFd << ")\n";
        }
        return true;
    }
    else if (s == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Temporarily cannot write: no es error fatal.
            return true;
        }
        // Error serio: marca cerrado para cleanup
        std::cerr << "[Error] send() fallo (fd: " << _clientFd << "): " << strerror(errno) << "\n";
        _closed = true;
        return false;
    }
    else
    { // s == 0
        // peer cerró la conexión
        _closed = true;
        return false;
    }
}

/*
flushWrite() es clave en el manejo del envío no bloqueante

Objetivo: intentar enviar (una llamada a send()) los datos pendientes del _writeBuffer del cliente, en varias tandas si hace falta, y de mantener el estado correcto (qué parte ya se envió, si el socket está listo, si hay que cerrar...). Manejar EAGAIN y errores. Actualizar _writeOffset y _lastActivity.

Devuelve true si no hubo error fatal (aunque quede pendiente). Devuelve false si ocurrió un error grave y el cliente queda marcado _closed = true.

1.
if (_writeOffset >= _writeBuffer.size())
{
    _writeBuffer.clear();
    _writeOffset = 0;
    return true;
}
👉 Significa: “Si ya envié todo lo que había en el buffer…”

Entonces:
    Limpia el buffer (ya no necesitamos guardar nada).
    Resetea _writeOffset (la posición dentro del buffer).
    Devuelve true → “todo enviado correctamente”.

🧩 Esto evita que intentes enviar cuando ya no hay nada pendiente.


2.
const char *buf = _writeBuffer.data() + _writeOffset;
size_t remaining = _writeBuffer.size() - _writeOffset;

👉 Aquí se calculan los datos que faltan por enviar:
    data() te da un puntero al contenido del std::string (en este caso, usado como buffer).
    Sumamos _writeOffset → saltamos los bytes ya enviados.
    remaining = cuántos bytes quedan.

    Ejemplo:
        _writeBuffer = "HTTP/1.1 200 OK\r\n..."
        _writeOffset = 10
        → buf apunta al byte 10
        → remaining = tamaño_total - 10


3.
ssize_t s = send(_clientFd, buf, remaining, 0);

👉 send() intenta escribir esos bytes en el socket del cliente.
Pero en modo no bloqueante, send() puede:
    devolver >0 → se enviaron s bytes;

    devolver -1 con errno = EAGAIN o EWOULDBLOCK → el socket no está listo para escribir (tendrás que esperar a POLLOUT);

    devolver -1 con otro errno → error grave;

    devolver 0 → el cliente cerró la conexión.


4.
Si se enviaron bytes...alignas

if (s > 0) {
    _writeOffset += static_cast<size_t>(s);
    _lastActivity = time(NULL);
    if (_writeOffset >= _writeBuffer.size()) {
        _writeBuffer.clear();
        _writeOffset = 0;
    }
    return true;
}

👉 Se actualiza el progreso:
    Avanza _writeOffset tantos bytes como se enviaron.

    Actualiza _lastActivity (último uso → útil para timeout).
        _lastActivity Es un timestamp (marca de tiempo) que guarda el último momento en que el cliente hizo algo “activo”:
            envió datos (lectura en readRequest)
            o recibió datos (envío en flushWrite)
        ¿Por qué time(NULL)? time(NULL) devuelve el tiempo actual en segundos desde 1970 (epoch).
            “Actualiza el reloj interno de este cliente: acaba de hacer algo.”
        El servidor lo usa para detectar clientes inactivos (idle connections).
        Por ejemplo, si un cliente se conecta y nunca termina de enviar su petición, o mantiene viva la conexión sin actividad, queremos cerrarla después de cierto tiempo.

    Si ya se envió todo → limpia el buffer.

    Devuelve true: “todo bien, seguimos”.

🧩 Esto permite enviar la respuesta en trozos, si el sistema solo deja enviar parte (por ejemplo, 4 KB cada vez).


5.
Si send() devuelve error temporal...

else if (s == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Temporarily cannot write
        return true;
    }

    👉 No es un error “mortal”:
    simplemente significa que el socket no puede enviar ahora (el buffer de salida del kernel está lleno).
    El poll() volverá a despertar este cliente cuando se pueda escribir (POLLOUT).

6.
Error real o cierre remoto...

std::cerr << "[Error] send() fallo (fd: " << _clientFd << "): " << strerror(errno) << "\n";
_closed = true;
return false;

👉 En cualquier otro caso, send() falló por una razón seria (cliente desconectado, error de red, etc.),
o devolvió 0 → el peer cerró la conexión.

Entonces marcamos _closed = true para que el servidor lo elimine más tarde.


Nota: flushWrite() solo hace una llamada a send() por invocación en esta versión (podrías hacer un while para intentar mandar todo en loops, pero con non-blocking es suficiente intentar una vez; si queda, poll te avisará con POLLOUT).

*/

/*
VERSIÓN ANTIGUA

bool Client::sendResponse(const std::string &msg)
{
    if (send(_clientFd, msg.c_str(), msg.size(), 0) < 0)
    {
        std::cerr << "[Error] Fallo al enviar respuesta al cliente (fd: " << _clientFd << ")\n";

        _closed = true; // Marcamos al cliente como cerrado para que el servidor deje de usarlo
        return false;
    }
    std::cout << "[Info] Respuesta enviada al cliente (fd: " << _clientFd << ")\n";
    return true;
}


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
FIN VERSION ANTIGUA*/

bool Client::isClosed() const
{
    return _closed;
}

/*
Comprueba si la conexión con este cliente ya se ha cerrado (por error o desconexión).
Se usa para que el servidor sepa si debe eliminar este cliente de la lista activa o no seguir intentando leer/escribir.
*/

void Client::markClosed()
{
    _closed = true;
}

/*
Por qué hacerlo así
    _closed sigue siendo privado, por lo tanto:
        Solo Client puede modificar su estado interno.
        Server solo puede pedirle “ciérrate”, no cambiarlo a lo bruto.

    Evita inconsistencias (por ejemplo, que el Server cierre el socket mientras el Client aún cree que está abierto).
*/

bool Client::isRequestComplete() const
{
    return _requestComplete;
}

const HttpRequest &Client::getHttpRequest() const
{
    return _httpRequest;
}

/*
getHttpRequest()

Sí es útil y limpio tenerlo, aunque tampoco obligatorio.
Cuando el Server (u otra parte del código) quiera acceder al contenido ya parseado, puedes hacer:

const HttpRequest &req = client.getHttpRequest();
std::cout << req.getMethod() << " " << req.getPath() << std::endl;


Si no lo tienes, tendrías que hacer algo feo tipo:

client._httpRequest.getPath(); // ❌ acceso directo a miembro privado


Así que getHttpRequest() sirve como interfaz de acceso controlado.
Conclusión: es buena práctica mantenerlo, aunque no imprescindible.
*/