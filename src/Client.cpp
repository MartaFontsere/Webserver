#include "Client.hpp"
#include <arpa/inet.h> // inet_ntoa()

/*
¿Por qué necesitamos Client.cpp?

Cuando tu servidor recibe una conexión (accept()), obtiene un nuevo file descriptor (FD) que representa a ese cliente específico.
Pero el servidor puede tener muchos clientes conectados al mismo tiempo.
→ Por tanto, necesitamos una forma clara de guardar y gestionar la información de cada cliente: su FD, su estado (si está leyendo o escribiendo), lo que ha enviado, lo que hay que responderle, etc.

La clase Client sirve justo para eso: encapsula todo lo que pasa con un cliente concreto dentro de un objeto.
Así evitamos caos y código duplicado dentro del servidor.
*/

Client::Client(int fd, const sockaddr_in &addr) : _clientFd(fd), _addr(addr), _closed(false)
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
    int bytesRead = recv(_clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0)
    {
        _closed = true;
        return false;
    }
    buffer[bytesRead] = '\0';
    _request += buffer;
    return true;
}

/*
➤ Qué hace:
Creamos un buffer temporal donde guardaremos los bytes que recibimos.
Tamaño 1024 bytes (1 KB).

Llamamos a recv() para leer datos del socket del cliente.
    _fd → socket del cliente.
    buffer → dónde guardar los datos.
    sizeof(buffer) - 1 → dejamos un byte libre para añadir '\0'.
    0 → flags (aquí no usamos ninguno especial).

👉 Si el cliente ha cerrado la conexión o hay error, recv() devuelve 0 o -1.

⚠️ if (bytesRead <= 0)
Si no hay datos o error:
    Marcamos _closed = true
    Y hacemos return (dejamos de procesar).

buffer[bytesRead] = '\0';
    Añadimos el carácter nulo al final, para que buffer sea una cadena C válida (char* terminado en \0).

_request += buffer;
    Guardamos los datos leídos en la petición completa del cliente (_request es un std::string).
    Así podemos recibir datos por partes si la petición llega fragmentada.

Si encuentra el delimitador \r\n\r\n, significa que la cabecera HTTP está completa (ya se ha recibido la petición entera) --> esto se controlara en el bucle del servidor o una funcion processClient()
    readRequest() no necesita saber nada del protocolo HTTP,
    solo acumula los datos recibidos.
    Y el “cerebro” del servidor decide cuándo esa petición está lista para procesar.

➤ Por qué es necesario:
    Las peticiones HTTP no siempre llegan de una sola vez.
    Un cliente puede enviar una parte ahora y otra dentro de unos milisegundos.
    Este método permite leer de forma incremental hasta tener la petición completa.

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

    \r\n\r\n es el final estándar de las cabeceras HTTP.

    _request acumula lo leído porque puede venir por partes.
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