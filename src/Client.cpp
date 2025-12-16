#include "Client.hpp"
#include <arpa/inet.h> // inet_ntoa()
#include <iostream>    // para imprimir mensajes
#include <sys/stat.h>
#include <fstream> // para ifstream
#include <sstream> // para ostringstream

#include <fcntl.h>   // open
#include <unistd.h>  // close, read
#include <sstream>   // ostringstream
#include <cerrno>    // errno
#include <limits>    // numeric_limits
#include <algorithm> // std::min

#include "Autoindex.hpp"

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
    mimeTypes.insert(std::make_pair("html", "text/html"));
    mimeTypes.insert(std::make_pair("css", "text/css"));
    mimeTypes.insert(std::make_pair("js", "application/javascript"));
    mimeTypes.insert(std::make_pair("png", "image/png"));
    mimeTypes.insert(std::make_pair("jpg", "image/jpeg"));
    mimeTypes.insert(std::make_pair("jpeg", "image/jpeg"));
    mimeTypes.insert(std::make_pair("gif", "image/gif"));
    mimeTypes.insert(std::make_pair("svg", "image/svg+xml"));
    mimeTypes.insert(std::make_pair("ico", "image/x-icon"));
    // HARDCODEADO, ESTO IRA EN EL CONFIGFILE SUPONGO
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
    if (bytesRead < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Si devuelve esto, no sigfnifica error, no hay datos ahora mismo (socket non-blocking). Por eso no tenemos que cerrar el socket en este caso, a diferencia del resto
            return true;
        }
        std::cerr << "[Error] Fallo al leer del cliente con recv() (fd: " << _clientFd << "): " << strerror(errno) << "\n";
        _closed = true;
        return false;
    }
    else if (bytesRead == 0)
    {
        // cliente cerró la conexión por su lado
        std::cout << "[Info] Cliente (fd: " << _clientFd << ") cerró la conexión normalmente.\n";
        _closed = true;
        return false;
    }

    // bytesRead > 0
    std::cout << "\nEmpezando a leer la Request (fd: " << _clientFd << ").\n";
    _rawRequest.append(buffer, bytesRead);

    std::cout << "  # Request recibida (fd: " << _clientFd << "):\n"
              << _rawRequest;

    _lastActivity = time(NULL);

    // Intentamos parsear la request actual
    std::cout << "[Debug] Parseando request del cliente fd " << _clientFd << std::endl;
    if (_httpRequest.parse(_rawRequest))
    {
        // ✅ Verificar SI el parse fue exitoso PERO con error de tamaño
        if (_httpRequest.isBodyTooLarge())
        {
            _httpResponse.setErrorResponse(413);
            applyConnectionHeader();
            _requestComplete = true; // ✅ Marcar como completa PARA RESPONDER
            return true;             // No es error fatal, hay respuesta que enviar
        }
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

// HELPPERS
void Client::applyConnectionHeader()
{
    if (_httpRequest.isKeepAlive())
        _httpResponse.setHeader("Connection", "keep-alive");
    else
        _httpResponse.setHeader("Connection", "close");
}

bool Client::validateMethod()
{
    const std::string &method = _httpRequest.getMethod();
    if (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD")
    {
        // Respondemos con 405 Method Not Allowed (contenido + headers dentro de HttpResponse)
        _httpResponse.setErrorResponse(405);
        applyConnectionHeader(); // Así el servidor maneja keep alive corretamente tanto en rutas validas como en invalidas (erroes), siempre coherente
        return false;
    }
    return true;
}

static int hexVal(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    return -1;
}

std::string Client::urlDecode(const std::string &encoded, bool plusAsSpace) const
{
    std::string decoded;
    decoded.reserve(encoded.size()); // Reservar memoria para evitar realocaciones

    for (size_t i = 0; i < encoded.size(); ++i) // Recorrer cáda caracter de la cadena
    {
        char c = encoded[i];
        if (c == '%' && i + 2 < encoded.size())
        {
            int highNibble = hexVal(encoded[i + 1]); // guardamos el valor después de %
            int lowNibble = hexVal(encoded[i + 2]);  // guardamos el valor dos veces después de %
            if (highNibble >= 0 && lowNibble >= 0)
            {
                decoded.push_back(static_cast<char>((highNibble << 4) | lowNibble)); // pusheamos los dos valores convertidos a hexadecimal haciendo movimiento de bits, para reconstruir el byte y decodificarlo (pasar de %2B, a high Nibble 2 y lowNibble 11, en hexadecimal, y al juntarlo en bits sea 2 → 0010 y 11 → 1011, y por lo tanto 0010 1011 = 0x2B = '+', decodificado)
                i += 2;                                                              // saltamos los dos hex procesados
            }
            else
            {
                // secuencia mal formada: conservador → dejamos '%' literal
                decoded.push_back('%');
                // no saltamos, así G y Z se procesarán en siguientes iteraciones
            }
        }
        else if (c == '+' && plusAsSpace) // TODO: se tiene que detectar en el parser de la request, si
        {
            // Solo convertir '+' → ' ' si explícitamente pedimos plusAsSpace=true.
            decoded.push_back(' ');
        }
        else
        {
            decoded += encoded[i];
        }
    }
    return decoded;
}

/*
¿Por qué existen urlEncode y urlDecode?
    Cuando un navegador envía una URL, no puede enviar caracteres especiales tal cual, siempre envía el path codificado.

    Esto NO es válido en una URL:
        /file with spaces.txt

    El navegador lo convierte automáticamente en:
        /file%20with%20spaces.txt

    Esto pasa siempre, independientemente de que escribas la URL a mano, hagas clic, vengas de autoindex... al servidor siempre le llega codificado. Por eso hay que decodificar

    Ejemplos de caracteres problemáticos:
        espacio
        á é í ó ü
        # (marca fragmentos)
        ? (abre query string)
        / (separador)
        % (inicio de codificación)
        : (protocolo)
        ;
        "

    Si los enviara tal cual, rompería la sintaxis del protocolo.

Solución del estándar: URL encoding (RFC 3986)
    La URL debe codificar esos caracteres raros como:
        %XX   ← valor hexadecimal del byte

    Ejemplos:
        "hola mundo" → hola%20mundo
        ñ           → %C3%B1  (UTF-8)
        ?           → %3F
        #           → %23

    Esto significa que cuando el servidor recibe una URL, NO es la URL real:
    es una versión escapada → tu servidor debe decodificarla para trabajar con rutas reales del sistema de archivos.

¿POR QUÉ ES IMPORTANTE PARA WEBSERV?
    Porque sin esto:

        /hola%20marta.txt → buscarías un archivo literal con %20 en el nombre
        (y fallaría con 404)

        consulta GET con parámetros ?name=Marta+Fontseré
        → recibirías Marta+Fontseré en vez de Marta Fontseré

        autoindex mostrando rutas tendría enlaces rotos

        seguridad: ataques de path traversal pueden venir codificados:
            ..%2F..%2Fetc/passwd

    Por eso es OBLIGATORIO para cualquier servidor web serio.

¿Y si el archivo se llama literalmente file%20.txt?
    Archivo real:
        file%20.txt

    Para pedirlo correctamente:
        El % debe codificarse como %25

            /file%2520.txt

        Decodificación:
             %25 → %

        Resultado final:
            file%20.txt

Qué es una query string?
    Una query string es la parte opcional de la URL que va después del ?

    Ejemplo:
        /search?q=hello+world&page=2
               ↑
               query string

    La URL se divide así:
        /search          → PATH
        ?q=hello+world   → QUERY STRING

    👉 NO son lo mismo
    👉 Se procesan distinto
    👉 Se codifican distinto

PATH vs QUERY

| Parte     | Qué es                | Para qué se usa           |
| --------- | --------------------- | ------------------------- |
|   PATH    | Identifica el recurso | Archivo / directorio      |
|   QUERY   | Parámetros            | Búsquedas, filtros, flags |

Ejemplo:
    /images/my photo.jpg?size=large

    PATH → /images/my photo.jpg
    QUERY → size=large

    El archivo es el mismo, cambie lo que cambie la query

¿Puede llegar una query string al webserver?
    Sí, totalmente.
    Cualquier request HTTP puede traerla:
        GET /file.txt?download=true HTTP/1.1

    Tu parser HTTP debería separar:
        path → /file.txt
        query → download=true

    Importante:
        La query NO forma parte del path del archivo.
        El filesystem no debe verla.

¿Y el carácter +? (esta es la trampa)
    + NO significa espacio en el PATH

    En URLs:
        PATH → espacios = %20
        QUERY → espacios = + (solo en form encoding)

Reglas definitivas para tu webserver (guárdalas)
    ✔ PATH
        Siempre viene URL-encoded
        Espacios → %20
        + es literal
        Decodifica %XX
        NO conviertas + → space
        Decodifica antes de sanitizePath

    ✔ QUERY STRING
        Espacios pueden venir como +
        Decodifica %XX
        Convierte + → space

    ✔ Autoinde
        Genera URLs codificadas (urlEncode)
        Usa %20 para espacios

CÓDIGO:
    Objetivo:
        Tomar una cadena así:
            /hola%20marta/archivo%2Etxt

        y convertirla en:
            /hola marta/archivo.txt

std::string decoded;
    Se crea la cadena que devolveremos, donde iremos añadiendo los caracteres ya decodificados.

decoded.reserve(encoded.size());
    Reservamos capacidad para decoded igual al tamaño de la cadena de entrada.
    Por qué: evita realocaciones internas al push_back/operator+= y mejora rendimiento.
    Nota: el tamaño final nunca será mayor que encoded.size() (de hecho suele ser ≤), así que es una reserva razonable.

for (size_t i = 0; i < encoded.size(); ++i)
    Recorre cada carácter de la cadena

    🔸 Caso 1 — detecta %XX (el inicio de una secuencia percent-encoded)
        if (encoded[i] == '%' && i + 2 < encoded.size())

        Esto significa:
            encoded[i] == '%' → el carácter % indica codificación, por lo que indica que viene una secuencia %XX
            i + 2 < encoded.size() → faseguramos que hay al menos dos caracteres hex detrás (% + 2 hex) para no salirnos del buffer
        Importante: si hay un % al final sin dos hex, este if será falso y se tratará más abajo como carácter normal

        int value = 0;
            Variable donde almacenaremos el valor numérico resultante de los dos dígitos hex.

        Lee esos dos caracteres:
        std::istringstream hexStream(encoded.substr(i + 1, 2));
            Creamos un istringstream con los dos caracteres hex (por ejemplo "20"). substr(i+1,2) toma los dos caracteres después del %.

        if (hexStream >> std::hex >> value)
            Intentamos leer desde el stream interpretando los caracteres como hexadecimal (std::hex) y asignarlo a value.

        decoded += static_cast<char>(value);
            Si la lectura hex fue correcta, convertimos value a char y lo añadimos a decoded.

        i += 2; // saltamos los dos hexadecimales que acabamos de procesar. El for incrementará i otra vez, por lo que el siguiente índice será el caracter posterior a la codificación

    🔸 Caso 2 — detecta + - ESTO AL FINAL NO LO HACEMOS, EN DECODIFICACION DE PATH NO TIENE SENTIDO, ES PARA QUERY STRINGS
        else if (encoded[i] == '+')

            En solicitudes HTML form (application/x-www-form-urlencoded) los espacios en la query string se codifican como +.

            En paths propiamente dichos los espacios deben codificarse como %20. Pero por compatibilidad con clientes o formularios, convertir + a espacio es sensato.

        Aunque no se usa en rutas, es habitual, así que lo interpretamos como ' '.

        ! Precaución: Algunos prefieren NO convertir + cuando se decodifica el path y sólo aplicarlo a query — depende del diseño. Tu implementación opta por compatibilidad universal.

    🔸 Caso 3 — cualquier otro carácter
        decoded += encoded[i];
            Si no requiere decode, lo dejamos igual.

Devolvemos la cadena decodificada.

*/

std::string Client::getDecodedPath() const
{
    // PATH → filesystem → + NO es espacio
    return urlDecode(_httpRequest.getPath(), false);
}

std::string Client::getDecodedQuery() const
{
    // QUERY → parámetros → + SÍ es espacio
    return urlDecode(_httpRequest.getQuery(), true);
}

// Devuelve "__FORBIDDEN__" si detecta path traversal o ruta inválida
// Devuelve "/" si path es "/".
std::string Client::sanitizePath(const std::string &decodedPath)
{
    if (decodedPath.empty())
        return std::string("/"); // si nos piden una ruta vacía, servimos raíz

    // Debe empezar por '/'
    if (decodedPath[0] != '/')
        return "__FORBIDDEN__";

    std::vector<std::string> allParts;
    bool endsWithSlash = (decodedPath.size() > 1 && decodedPath[decodedPath.size() - 1] == '/');

    size_t i = 1; // saltamos la primera '/' para evitar vacío al dividir
    while (i <= decodedPath.size())
    {
        size_t j = decodedPath.find('/', i);
        std::string part;
        if (j == std::string::npos)
        {
            part = decodedPath.substr(i);
            i = decodedPath.size() + 1;
        }
        else
        {
            part = decodedPath.substr(i, j - i);
            i = j + 1;
        }
        if (part.empty() || part == ".")
        {
            // ignorar
            continue;
        }
        else if (part == "..")
        {
            if (allParts.empty())
            {
                // intento de escapar por encima del root -> prohibido
                return std::string("__FORBIDDEN__");
            }
            allParts.pop_back();
        }
        else
            allParts.push_back(part);
    }

    // Reconstruir ruta limpia
    std::string cleanPath = "/";
    for (size_t k = 0; k < allParts.size(); ++k)
    {
        cleanPath += allParts[k];
        if (k + 1 < allParts.size())
            cleanPath += "/";
    }

    // Si la ruta termina en '/', añadimos index.html
    // if (!path.empty() && path[path.size() - 1] == '/')
    // cleanPath += (cleanPath.size() > 1 ? "/index.html" : "index.html");

    // Mantener barra final si la tenía
    if (endsWithSlash && cleanPath[cleanPath.size() - 1] != '/')
        cleanPath += "/";

    // Si quedó vacío, devolver "/"
    // TODO: REVISAR, necesario?
    // if (cleanPath.empty())
    // return "/";

    return cleanPath;
}
/*
Aquí solo decides si la ruta es inválida o peligrosa.
No generas respuesta aún.
Solo transformas un input a un resultado lógico.

Esto lo hace predecible, testeable y modular.

"__FORBIDDEN__" es un sentinel value.

Otros servidores usan:
    return false;
    throw ForbiddenException();
    códigos enum como:
        enum PathStatus { OK, FORBIDDEN, NOT_FOUND };

Pero dado que tu flujo es simple, "__FORBIDDEN__" funciona perfectamente.

Si luego quieres mejorarlo:
    ✔️ puedes cambiarlo a un enum
    ✔️ o a un objeto tipo PathResult

Pero de momento asi está perfecto

¿Qué es un "path traversal"?
    Es un tipo de ataque en el que un cliente intenta salir del directorio permitido usando rutas como:
        /../../etc/passwd
        /imagenes/../secretos/clave.txt
    En lenguaje de rutas, .. significa “sal del directorio actual”.

    El navegador envía la ruta, y si tu servidor la concatena sin validar:
        WWW_ROOT + "/../../etc/passwd"

    ➡️ El atacante podría acceder a archivos fuera de tu web root.

    Esto es crítico en cualquier webserver.

CÓDIGO:
Tu función pretende hacer una primera limpieza rápida

0. Firma
std::string Client::sanitizePath(const std::string &path)
    Entrada: path tal como viene en la petición HTTP (por ejemplo "/index.html", "/a/../b", "/foo//bar").

    Salida: una ruta "limpia" normalizada (por ejemplo "/a/b" o "/index.html"), o la cadena especial "__FORBIDDEN__" para indicar que el path es inválido/peligroso.

1. Si el path está vacío → devolvemos "/"
if (path.empty())
    return "/";

    Simplemente evita rutas raras tipo:
    GET HTTP/1.1 → path vacío → servimos raíz /.

2. Siguiente paso: “Asegurarnos que empieza por /”
    if (cleanPath[0] != '/')
        cleanPath = "/" + cleanPath;

    Esto es simplemente un saneamiento sintáctico.
    Si la primera letra no es '/', consideramos la petición inválida o maliciosa.
    Tu servidor espera rutas absolutas, no relativas.
        No intentamos arreglar rutas relativas: la especificación HTTP espera un request-target absoluto.
    No intentamos arreglar rutas relativas: la especificación HTTP espera un request-target absoluto.
    ! Resultado: devolver "__FORBIDDEN__" para que el controlador principal genere un 403 (o 400) - REVISAR

3. De inicio rechazaba cualquier ".." explícito
    if (path.find("..") != std::string::npos)
        return "__FORBIDDEN__";

    Esto es una defensa básica contra path traversal.
    Si alguien pide:
        GET /../etc/passwd

    ➡️ detectamos ".."
    ➡️ devolvemos "__FORBIDDEN__"

        Es correcto y normal bloquear cualquier .. en sanitizePath, aunque haya casos donde NO saldría del root. Simplificado. A nivel de diseño no está permitido, y no podrias comprobar de forma segura si escapa del root, asi que siempre se bloquea

    Pero OJO: esto NO seria suficiente en un servidor mas avanzado, porque existen variantes y trucos:
        /.%2e/ (URL encoded)
        %2e%2e/ (URL encoded double dot)
        /foo/../bar
        /foo//bar
        /foo/./bar
        /foo//../bar

        O incluso:
            /something/%2e%2e/secret

        Cuando el servidor decodifica internamente %2e → “.”
        El atacante consigue bypass.

        Por eso la función sola no protege del todo. SanitizePath filtra basura obvia

3.1. Versión mejorada, no devolvemos forbiden siempre, hay que gestionar varias situaciones

| CASO                                       | ¿QUÉ DEVUELVE `sanitizePath()`? |
| ------------------------------------------ | ------------------------------- |
| Path vacío                                 | `"/"`                           |
| Path no empieza por `/`                    | `"__FORBIDDEN__"`               |
| Path intenta escapar (`..` escapando root) | `"__FORBIDDEN__"`               |
| Path válido pero con `.` o `..` internos   | ruta normalizada                |
| Path termina en `/`                        | añadir `index.html`             |

Para ello vamos a dividir la ruta en partes
    Porque es la única forma fiable de entender realmente qué significa una ruta, especialmente cuando incluye cosas como:
        . (directorio actual)
        .. (directorio padre)
        // (doble slash)

    rutas con basura (ej: /foo/../bar/././baz)

    El servidor no puede adivinar qué significa una ruta solo buscando substrings como "..".
    Pero sí puede interpretarla correctamente si la separa en segmentos y los procesa.

¿Qué problema tiene la versión antigua?

    Antes hacías algo como:
        Si hay .. → FORBIDDEN
        Si acaba en / → añade index.html
        Quitar dobles barras quizá
        Y poco más

    ¿Y qué pasa?

        Problema 1 → Falsos positivos
            Hay rutas con .. que son seguras: /imagenes/../foto.jpg
                Esto NO sale del root.
                La versión antigua lo prohibía aunque fuera legítimo.

        Problema 2 → Falsos negativos (inseguridad real)
            Un atacante puede usar combinaciones:
                /a/b/../../../../../etc/passwd

            O encoded:
                /a/%2e%2e/%2e%2e/etc/passwd

            O mezclas raras:
                /foo/bar/..///../secret

            Si te limitas a find(".."), esto se te cuela seguro.

        Problema 3 → Navegación real del filesystem
            El servidor debe comportarse como si navegara por carpetas.
            Eso significa:
                "." → no hace nada
                ".." → subir un nivel
                "//" → no significa nada especial → se ignora el segmento vacío

            La única forma de simularlo es mantener una pila de segmentos, igual que hace el sistema operativo.

    La nueva lógica aporta:
        Normalización correcta del path
            Te toma algo raro como: /a/./b/../c///d/
            y lo convierte en: /a/c/d/index.html

        Seguridad real
            Dividir en partes y manejar ".." con una pila permite decir:
                ".." cuando tienes partes previas → OK
                (/a/b/../c → /a/c)

                ".." cuando NO hay nada previo → FORBIDDEN
                (/../etc/passwd → ataque)

            Es exactamente lo que hace un path canonicalizer real.

        Compatibilidad con clientes reales
            Los navegadores pueden enviar rutas con:
                ./folder
                folder/../other
                folder//file
                folder///././image
                finales en /
                múltiples barras
                rutas vacías

            Los servidores utilizan siempre la lógica basada en segmentos.

        Separación de responsabilidades
            sanitizePath() limpia y normaliza la ruta
            Otra función (como buildFullPath()) une esa ruta con WWW_ROOT y hace stat()
            Otra función decide si sirve el archivo o devuelve 404/403

            Mucho más limpio, más profesional y más fácil de razonar.

3.1.1. Preparación para dividir la ruta
    std::vector<std::string> parts;
    size_t i = 1; // evitar elemento vacío

parts almacenará los segmentos limpios de la ruta (p. ej. ["blog", "2025", "post.html"]).

size_t i = 1 porque path[0] es '/'. Empezamos a buscar desde el carácter 1 para no crear un segmento vacío antes del primer /.

3.1.2 Bucle principal — dividir y procesar segmento a segmento
while (i <= path.size())
{
    size_t j = path.find('/', i);
    std::string part;

    if (j == std::string::npos)
    {
        part = path.substr(i);
        i = path.size() + 1;
    }
    else
    {
        part = path.substr(i, j - i);
        i = j + 1;
    }
    ...
}

path.find('/', i) busca la próxima / empezando desde i.
    Si no la encuentra, find devuelve std::string::npos.

Si j == npos:
    part = path.substr(i) toma desde i hasta el final.
    i = path.size() + 1 fuerza la salida del while en la siguiente comprobación.

Si j != npos:
    part = path.substr(i, j - i) toma el segmento entre i y j-1.
    i = j + 1 salta justo después de la / encontrada, para la próxima iteración.

En resumen: esto parte la ruta por / produciendo cada componente en part.

Ejemplo: "/a/b/c" → secuencia de part: "a", "b", "c".

3.1.3. Ignorar vacíos y "."
if (part.empty() || part == ".")
{
    continue;
}

Casos que generan part.empty():
    entradas con // (doble slash) producen partes vacías.
    rutas que empiezan o acaban con / pueden producir vacíos si no lo manejas.

"." es el segmento que significa “el directorio actual” y no aporta nada; lo ignoramos.

continue salta a la siguiente iteración sin añadir nada a parts.

Ejemplo: "/foo//bar/./baz" → ignorará la parte vacía entre // y ignorará ..

3.1.4. Tratar ".." (subir un nivel)
else if (part == "..")
{
    if (parts.empty())
        return "__FORBIDDEN__"; // escape detected
    parts.pop_back();
}

".." significa “subir un nivel” (ir al padre).

parts.pop_back() elimina el último segmento almacenado (simula subir un nivel).

Importante: si parts está vacío y aparece .., eso significa que el cliente intenta salir por encima de la raíz (p.e. "/../etc/passwd"). Se considera malicioso y la función devuelve "__FORBIDDEN__".

    Este return es la política segura cuando no puedes (o no quieres) resolver la ruta real con funciones del sistema.

Ejemplo seguro: "/a/b/../c" → parts pasa de ["a","b"] a ["a"] luego se añade "c" → ["a","c"].

3.1.5. Añadir segmento normal
else
{
    parts.push_back(part);
}

Si part no es vacío, ni ".", ni "..", se añade como componente válido a parts.

4. Reconstruir la ruta limpia
std::string clean = "/";
for (size_t k = 0; k < parts.size(); ++k)
{
    clean += parts[k];
    if (k + 1 < parts.size())
        clean += "/";
}

Construye la cadena final empezando con '/'.

Añade cada segmento separados por '/'.

Resultado: ruta normalizada sin . ni .. ni //.

Ejemplo: parts = ["blog","post.html"] → clean = "/blog/post.html".

5.Si termina en '/', añade index.html
    if (cleanPath[cleanPath.size() - 1] == '/')
        cleanPath += "index.html";

Si la ruta original acababa en '/', asumimos que el cliente está pidiendo la “carpeta”, por lo que añadimos index.html.

clean.size() > 1 se usa para evitar //index.html cuando la ruta limpia es solo "/".
    Si cleanPath == "/", concatenar "index.html" resulta "/index.html".
    Si cleanPath == "/foo", concatenar "/index.html" resulta "/foo/index.html".

Ejemplos:

path = "/" → cleanPath inicialmente "/", acaba en '/' → result "/index.html".

path = "/blog/" → cleanPath "/blog" → add "/index.html" → "/blog/index.html".

Comportamiento típico de nginx y apache.

6. Devolver cleanPath
Devuelve la ruta ya normalizada y lista para concatenar con WWW_ROOT y probar existencia con stat().

Pasos reales y seguros:
    safe = sanitizePath(path)
    full = WWW_ROOT + safe
    real = realpath(full) ← esto normaliza y resuelve ‘..’
    Compruebas si real empieza por WWW_ROOT

Cómo se usa en el resto del servidor

    processRequest() o serveStaticFile() llamará a sanitizePath(path).
        Si devuelve "__FORBIDDEN__" → responde 403.
        Si devuelve "/something" → concatena WWW_ROOT + clean → hace stat() sobre esa ruta.
            Si existe y es fichero → leer y servir.
            Si no existe → 404.

RESUMEN:
    sanitizePath() hace:
        Validación básica (prohibe "..")
        Sintaxis limpia (añadir '/') (normalizar formato)
        Añadir index.html
    Esto es preparar la ruta que viene del cliente, path limpio pero superficial.

    Lo que NO hace (y es normal):
        Resolver %2e → .
        Quitar . o ..
        Asegurar físicamente que el archivo ecstá dentro del root
        Detectar enlaces simbólicos -> acceso directo emmascarado
            Es decir: que una ruta aparentemente segura en realidad haga que tu servidor acabe sirviendo un archivo privado.
                /Users/marta/webserver/www/imagenes
                    → acceso directo → /Users/marta/Documentos/Privado
                Si alguien pide /imagenes/secreto.txt,
                tu servidor podría acabar sirviendo un archivo privado.

            Hay que tenerlo en cuenta para preguntarle al sistema:
                “Oye, esta ruta que me han pasado… ¿de verdad dónde termina?”
                “No me des la ruta tal cual está escrita: dame la ruta REAL del sistema.”
            Ejemplo:
                /Users/marta/webserver/www/imagenes → symlink → /Users/marta/Privado
                User pide: /imagenes/secretos.txt
                realpath te devuelve: /Users/marta/Privado/secretos.txt
                    ¡Esto está fuera del root!
                    ¡PELIGRO!
                    Aquí es donde entra “check contra WWW_ROOT”. Es decir, checkear que la ruta empieza con WWW_ROOT

    Al no poder usar la funcion realPath(), no podemos resolver todo esto 100%, pero hacemos aproximaciones

    Lo que se hace luego, en buildFullPath():
        Unir sanitizePath(path) con el WWW_ROOT
        Convertirlo a ruta real (realpath)
        Comprobar si se sigue dentro de WWW_ROOT
        Evitar accesos fuera del root

 */

std::string Client::buildFullPath(const std::string &cleanPath)
{
    // Si sanitize decidió que esto es inseguro, no seguimos
    if (cleanPath == "__FORBIDDEN__")
        return "__FORBIDDEN__";

    // Normalizamos WWW_ROOT: sin barra final
    std::string root = WWW_ROOT;
    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1);

    // cleanPath siempre viene ya con un '/' inicial garantizado por sanitize
    return root + cleanPath;
}

/*
Aquí solo construyes rutas del sistema de archivos.
Otro tipo de responsabilidad.

Respeta el principio SRP (una sola responsabilidad)
    sanitizePath() → valida y normaliza
    buildFullPath() → construye la ruta final
    serveStaticFile() → comprueba existencia, permisos, MIME, lectura, etc.

ROOT
    El root” = la carpeta raíz del servidor web = la carpeta que sí se puede enseñar.
    Así que el objetivo final es que ninguna ruta, ni limpia ni sucia, salga de esa carpeta.
    Tu servidor NUNCA debe permitir acceder a otra ruta que no sea desde el root

NOTA IMPORTANTE

 Mejor si WWW_ROOT es una ruta absoluta (p.e. /home/user/www). Si la config la deja relativa (ej: ./www), el servidor funcionará pero las comprobaciones de "escapado" son menos estrictas: es mejor definir WWW_ROOT absoluto en config. Si no puedes, documenta que config debe ser absoluto.

    La detección de 404, 403, 200, 301, etc.
    NO debe estar en buildFullPath().
    Eso va en serveStaticFile() cuando:
        haces stat
        compruebas si es directorio
        si existe el index
        si no existe → 404
        si no tienes permiso → 403

 */

void Client::serveStaticFile(const std::string &fullPath)
{
    std::cout << "entrooooooooooooooooooooo" << std::endl;
    // 1) Caso prohibido desde buildFullPath o sanitize
    if (fullPath == "__FORBIDDEN__")
    {
        _httpResponse.setErrorResponse(403);
        applyConnectionHeader();
        return; // indica respuesta de error preparada
    }

    // Comprobar existencia con stat()
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0)
    {
        // stat no pudo acceder: ENOENT → 404, EACCES → 403
        if (errno == EACCES)
            _httpResponse.setErrorResponse(403);
        else
            _httpResponse.setErrorResponse(404);
        applyConnectionHeader();
        return; // TODO: esto es necesario??? si se hace en el handle get...
    }

    // Protección contra archivos gigantes
    // 4) Validar tamaño
    if (fileStat.st_size < 0)
    {
        _httpResponse.setErrorResponse(500);
        applyConnectionHeader();
        return;
    }

    size_t size = static_cast<size_t>(fileStat.st_size);
    if (size > MAX_STATIC_FILE_SIZE)
    {
        _httpResponse.setErrorResponse(413); // Payload Too Large
        applyConnectionHeader();
        return;
    }

    // Leer archivo
    std::string content;
    if (!readFileToString(fullPath, content, fileStat.st_size))
    {
        // open/read error → revisar errno
        if (errno == EACCES)
            _httpResponse.setErrorResponse(403);
        else if (errno == ENOENT)
            _httpResponse.setErrorResponse(404);
        else if (errno == EFBIG)
            _httpResponse.setErrorResponse(413);
        else
            _httpResponse.setErrorResponse(500);

        applyConnectionHeader();
        return;
    }
    // MIME
    std::string mime = determineMimeType(fullPath);

    // Preparar respuesta OK
    std::ostringstream oss;
    oss << content.size();

    _httpResponse.setStatus(200, "OK");
    _httpResponse.setHeader("Content-Type", mime);
    _httpResponse.setHeader("Content-Length", oss.str());
    applyConnectionHeader();
    _httpResponse.setBody(content);

    std::cout << "[Client fd=" << _clientFd << "] Archivo servido: " << fullPath << "\n";
    return;
}

/*
Esta función intenta servir un fichero estático (leerlo del disco y preparar _httpResponse con su contenido y cabeceras).
Devuelve true si ha preparado correctamente la respuesta (200 OK con body), o false si se produjo un error y ya ha puesto una respuesta de error (403/404/413/500).

Qué es stat?
    stat es una llamada al sistema de Unix que te permite obtener información sobre un archivo o directorio: tamaño, permisos, tipo (fichero, directorio…), fechas, etc.

    Piensa en stat() como:
    “Oye kernel, cuéntame todo lo que sabes de este archivo.”

📌 ¿Qué devuelve exactamente stat?
La función:
    int stat(const char *path, struct stat *buf);

Rellena una estructura struct stat con datos como:

✔️ Tipo de archivo
    S_ISREG(st_mode) → fichero regular
    S_ISDIR(st_mode) → directorio
    S_ISLNK(st_mode) → enlace simbólico
    etc.

✔️ Permisos
    st_mode también contiene los permisos (rwx) del archivo.

✔️ Tamaño
    st_size → tamaño en bytes.

✔️ Fechas
    st_mtime → última modificación
    st_ctime → cambio de metadatos
    st_atime → último acceso

📁 ¿Para qué sirve en un webserver?
    Es básico para implementar:

    ✔️ 1. Comprobar si un path existe
        Si stat devuelve -1 con errno == ENOENT → 404 Not Found

    ✔️ 2. Saber si el path es un directorio
        Si es un directorio sin / final → 301 redirect
        Si es un directorio con / final → buscar índice (index.html)

    ✔️ 3. Saber si tienes permiso para leer el archivo
        Si !(st_mode & S_IROTH) → 403 Forbidden

    ✔️ 4. Saber el tamaño del archivo para enviar el header Content-Length

CÓDIGO

struct stat fileStat;
if (stat(fullPath.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode))
{
    _httpResponse.setErrorResponse(404);
    applyConnectionHeader();
    return false;
}

    stat() consulta al sistema de ficheros y rellena fileStat con metadatos (tamaño, permisos, si es directorio, timestamps, etc).

    stat(...) != 0 → stat falló (fichero no existe, permisos insuficientes, ruta inválida) → respondemos 404 Not Found.

    S_ISDIR(fileStat.st_mode) comprueba si lo que hay en fullPath es un directorio; si es directorio también devolvemos 404 (normalmente no servimos directorios como fichero).

    Importante: stat también devuelve errores por permisos (EACCES) — podrías devolver 403 en ese caso, pero aquí se normaliza a 404.

if (fileStat.st_size > MAX_STATIC_FILE_SIZE)
{
    _httpResponse.setErrorResponse(413); // Payload Too Large
    applyConnectionHeader();
    return false;
}
Antes de abrir y leer todo el archivo, nos aseguramos de que podamos soportarlo en memoria, sino salimos.

std::string mime = getMimeType(fullPath);

Calcula el tipo MIME (ej. text/html, image/png) a partir de la extensión del fullPath (ver tu getMimeType).

_httpResponse.setStatus(200, "OK");
_httpResponse.setHeader("Content-Type", mime);
_httpResponse.setHeader("Content-Length", std::to_string(content.size()));
applyConnectionHeader();
_httpResponse.setBody(content);

Construimos la respuesta con:

    200 OK

    Content-Type: <mime>

    Content-Length: <nbytes> — aquí se pone el tamaño exacto del body. IMPORTANTE: en C++98 std::to_string no existe; usa std::ostringstream para convertir size() a string (ver nota abajo).

    applyConnectionHeader() — añade la cabecera Connection según tu política (keep-alive o close) y posiblemente Keep-Alive con timeout/max.

    setBody(content) — coloca el contenido leído como body de la respuesta.

Conceptos nuevos que aparecen aquí (resumen)

    stat(): llamada POSIX que devuelve metadatos del fichero (tamaño, tipo, permisos, timestamps).

    S_ISDIR(mode): macro para comprobar si mode es un directorio.

    ifstream + ios::binary: abrir fichero en modo binario (sin transformaciones).

    MIME type: tipo de contenido que indica al cliente cómo interpretar el body (text/html, image/png...).

    Content-Length: número exacto de bytes del body; necesario si no usas chunked.

    applyConnectionHeader(): utilidad para añadir Connection (y posiblemente Keep-Alive) según tu política.

*/

/*
ACLARACIÓN

LLAMAR A READ FILE TO STRING SIN PROTECCIÓN PREVIA
        👉 Esto funciona perfectamente para ficheros pequeños o medianos.
        ❌ Pero se convierte en un peligro serio si el cliente pide ficheros enormes.

    Ejemplo típico:
        Te piden que sirvas un archivo de 2 GB.
        Tu servidor intenta hacer out.resize(2 * 1024 * 1024 * 1024)
        ¡Boom!
            Consumes toda tu RAM
            Matas al servidor
            El proceso es expulsado por el OOM Killer
            Cliente sin respuesta
            Server KO para todos los demás usuarios

        ➡️ Un servidor profesional nunca lee archivos grandes enteros en memoria.

    Para eso usaremos una constante que ponga un limite razonable para servir en memoria
        static const size_t MAX_STATIC_FILE_SIZE = 10 * 1024 * 1024; // 10 MB

    Significa:
        Solo acepto servir archivos pequeños mediante lectura completa.
        Si el archivo es mayor, NO lo leeré entero a memoria, sino que:
            O bien respondo 413 Payload Too Large
            O bien lo sirvo por streaming, trozo a trozo (lo veremos luego)
            O devuelvo un 404 como si no existiera (menos profesional)

        10 MB es un ejemplo. Podría ser:
            1 MB → muy estricto
            50 MB → razonable
            100 MB → más generoso pero arriesgado

        En servidores reales se usa un límite para proteger el servidor.

    Por eso hacemos la protección contra archivos grandes:
        if (static_cast<size_t>(size) > MAX_STATIC_FILE_SIZE)
            return false; // o marcar error 413

    ¿Por qué es necesaria esta mejora?
        ✔️ Evita que el servidor consuma toda la RAM
            Un atacante podría llamarte con:
                GET /video_HD_9GB.mp4 HTTP/1.1

            Sin límite → tu servidor muere.

        ✔️ Evita un DDoS involuntario
            Cualquier fichero enorme en tu /www puede tumbarte.
                    DDoS = Distributed Denial of Service.
                        Muchas máquinas (o un atacante simulando muchas) te envían peticiones diseñadas para bloquear, saturar o colapsar tu servidor, hasta dejarlo inutilizado.

        ✔️ Servidores reales usan límites
            NGINX:
                client_max_body_size
                proxy_buffer_size
                sendfile para evitar lectura a memoria

            Apache:
                LimitRequestBody
                LimitXMLRequestBody

    De primeras piensas, “Yo no voy a tener archivos gigantes en mi disco, así que no me afectaría, ¿no? No tengo que protegerlo”
        En principio sí, si tú controlas 100% qué ficheros hay en tu carpeta www/.

        Pero…
            El evaluador puede poner cualquier archivo en tu directorio.

            En tu máquina personal o en un servidor real, cualquier usuario con permiso podría subir un archivo enorme (upload, repositorio, backups, etc.)

            Y lo más importante:
            tu servidor no decide qué archivo existe: lo decide el sistema de ficheros.

            Aunque tú creas que no hay archivos grandes… sí podrían aparecer.

        Ejemplo realista
            Tú crees que tu carpeta solo tiene:
                /www/index.html (3 KB)
                /www/style.css (1 KB)

            Pero puede existir fuera de tu carpeta web pero dentro de la ruta accesible por error:
                /home/user/Descargas/Movie_4K_120GB.mkvEjemplo realista

            Tú crees que tu carpeta solo tiene:
                /www/index.html (3 KB)
                /www/style.css (1 KB)

            Pero puede existir fuera de tu carpeta web pero dentro de la ruta accesible por error:
                /home/user/Descargas/Movie_4K_120GB.mkv

            Si por un error en tu routing construyes ese path, tu servidor intenta leerlo → RAM muerta.

            Un atacante puede pedir cualquier ruta inventada. Si ese path casualmente existe en el disco (por cualquier motivo):
                copia de un ISO
                un backup
                un archivo olvidado
                algo generado por otro proceso
                un archivo que el evaluador pone para probarte

            …tu servidor intenta leerlo antes de decidir qué responder.
*/

bool Client::readFileToString(const std::string &fullPath, std::string &out, size_t size)
{
    // Abrir fichero (intentar no seguir symlinks si está disponible)
    int flags = O_RDONLY;
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW; // suma esta flag
#endif
    int fd = open(fullPath.c_str(), flags);
    if (fd < 0)
    {
        // errno queda establecido por open()
        return false;
    }

    out.clear();
    out.resize(size); // reservar memoria exacta

    size_t total = 0;
    while (total < size)
    {
        ssize_t bytesRead = read(fd, &out[total], size - total);
        if (bytesRead < 0)
        {
            if (errno == EINTR)
                continue; // reintentar

            // Error real
            close(fd);
            return false;
        }
        if (bytesRead == 0)
        {
            // EOF inesperado
            break;
        }
        total += static_cast<size_t>(bytesRead);
    }

    close(fd);

    // Si se leyó menos (EOF), ajustamos
    if (total < size)
        out.resize(total);

    return true;
}

/*
0. Firma de la función
    fullPath → ruta absoluta del fichero (ya garantizada, validada, sin "..", etc.).

    out → donde se guardará el contenido.

    size → el tamaño exacto del fichero, ya obtenido con stat() en serveStaticFile.

1. Abrimos el fichero de manera segura
        int flags = O_RDONLY;
    #ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
    #endif
        int fd = open(fullPath.c_str(), flags);

O_RDONLY -> Abrimos solo para leer.

O_NOFOLLOW -> Protege de este ataque:
    /www/index.html → es un symlink hacia /etc/passwd

        O_NOFOLLOW es una flag opcional de open() (no existe en todos los sistemas, por eso siempre la rodeamos con #ifdef O_NOFOLLOW).

        Su función: impedir que open() siga un symlink (enlace simbólico).

        Si open() detecta que el path final es un symlink fallará inmediatamente y pondrá: errno = ELOOP

Con O_NOFOLLOW, open() fallará, evitando fuga de archivos del sistema.
Es una protección opcional (solo existe en algunos sistemas), por eso va con #ifdef.

✔ El open devuelve un fd (file descriptor)

Si es < 0, hubo error.
    errno queda configurado, y serveStaticFile() se encargará de transformarlo en:
        403 si EACCES
        404 si ENOENT
        500 en otros casos

    La función NO decide el código → responsabilidad bien distribuida.

2. Reservamos memoria en el string
    out.clear(); -> Eliminamos contenido previo.
    out.resize(size); -> Reserva size bytes exactos para leer el fichero entero.

Esto es eficiente y evita realocaciones.

3. Bucle de lectura segura
    size_t total = 0;
    while (total < size)
    {
        ssize_t r = read(fd, &out[total], size - total);

read(fd, buffer, bytes)
Intenta leer hasta los bytes restantes (size - total).
Pero read no garantiza que lea todo:
puede devolver 1 byte, 200 bytes, 0, etc.
Por eso el bucle.

Caso 1 —> Error real de lectura
    if (bytesRead < 0)
    {
        if (errno == EINTR)
            continue;   // reintentar

                ✔ EINTR: señal del sistema interrumpió la lectura

                Esto pasa muy a menudo en servidores reales.

                Lo correcto es reintentar la lectura.

                Muy profesional que lo contemples.

    close(fd);
    return false;
        Cualquier otro error → devolvemos false y dejamos a errno contar la historia.

Caso 2 —> bytesRead == 0: EOF inesperado
    if (bytesRead == 0)
    {
        break;
    }

    Si nos llega un 0 antes de terminar el tamaño esperado:
    💥 El archivo se ha truncado, o está corrupto, o algo raro ha pasado.

    Lo tratamos como lectura incompleta (no como error total).
    Más tarde ajustamos el tamaño.

📈 Caso 3 — Lectura correcta
        total += static_cast<size_t>(bytesRead);

    Vamos acumulando bytes leídos.

4. Cerramos el descriptor
    close(fd);
Vital: no fugamos descriptores (puede agotar recursos del servidor).

5. Ajuste final si hubo EOF antes de tiempo
    if (total < size)
        out.resize(total);

    Porque reservamos size, pero si solo hemos leído total, quedan bytes basura al final.

    Así garantizamos que out.size() coincide con lo realmente leído.

*/

std::string Client::determineMimeType(const std::string &fullPath)
{
    size_t dot = fullPath.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string fileExtension = fullPath.substr(dot + 1);

    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(fileExtension);
    if (it != mimeTypes.end())
        return it->second;

    return "application/octet-stream";
}

/*
find_last_of('.') busca la última aparición de un punto en el nombre del archivo.
    Ej: "index.html" → dot = 5
    "archivo" → dot = npos (no hay punto)
        "application/octet-stream" es el tipo MIME genérico que se usa cuando no sabemos el tipo de archivo (no se reconoce la extensión). En HTTP es un MIME genérico para archivos binarios desconocidos.

fileExtension = path.substr(dot + 1) → obtiene la extensión del archivo (dot + 1 significa que hace el substring desde lo que hay justo despues del punto, hasta el final).
    "index.html" → fileExtension = "html"
    "archivo" → no hay extensión

mimeTypes es un std::map<std::string, std::string> con los tipos MIME conocidos:
*/

bool Client::processRequest()
{
    std::cout << "******************************* ENTRO" << std::endl;
    // 1) Reseteamos cualquier HttpResponse previa (estado limpio) -> Limpia HttpResponse previo
    _httpResponse = HttpResponse(); // crea un HttpResponse por defecto y lo copia en el miembro //? AQUÍ O AL ACABAR DE USARLO LO DEJAMOS LISTO PARA LA PRÓXIMA??? LA PRIMERA VEZ QUE SE USE YA SE CREA SOLO CON CLIENT, ASI QUE NO PASARÍA NADA

    // 2) Validar método HTTP (por ahora solo admitimos GET)
    if (!validateMethod())
        return true; // hemos generado una respuesta válida -> no es un fallo fatal, es un error que mandamos como respuesta, por lo que devolvemos true

    const std::string &method = _httpRequest.getMethod();
    std::cout << "******************************* LLEGO" << std::endl;

    if (method == "GET")
        return handleGet();
    else if (method == "HEAD")
        return handleHead();
    else if (method == "POST")
        return handlePost();
    else if (method == "DELETE")
        return handleDelete();

    std::cout << "******************************* LLEGO" << std::endl;

    // Esto no debería pasar
    _httpResponse.setErrorResponse(405);
    applyConnectionHeader();
    return true; // ✅ Correcto - respuesta de error configurada
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

bool Client::handleGet()
{
    // Obtener ruta en bruto y comprobar peligros
    std::string rawPath = _httpRequest.getPath();

    // 1. Decodificar
    std::string decoded = getDecodedPath();

    // 2. Sanitizar
    std::string sanitized = sanitizePath(decoded);

    std::cout << "******************************* Raw Path pedido:" << rawPath << std::endl;

    // Construir ruta real dentro de WWW_ROOT
    std::string fullPath = buildFullPath(sanitized);
    std::cout << "******************************* Full Path pedido:" << fullPath << std::endl;

    // Comprobar existencia con stat()
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0)
    {
        // stat no pudo acceder: ENOENT → 404, EACCES → 403
        if (errno == EACCES)
            _httpResponse.setErrorResponse(403);
        else
            _httpResponse.setErrorResponse(404);
        applyConnectionHeader();
        return true;
    }

    // 3) No aceptar directorios
    // if (S_ISDIR(fileStat.st_mode))
    // {
    //    _httpResponse.setErrorResponse(404);
    //    applyConnectionHeader();
    //    return false;
    //}

    // 3) NUEVO: Si es directorio delegar en Autoindex, que decide:
    //    1. ¿Existe un archivo por defecto? (index.html)
    //    2. Si existe → lo sirve
    //    3. Si NO existe pero autoindex está activado → genera listado de directorio
    //    4. Si NO existe y autoindex está desactivado → 403

    if (S_ISDIR(fileStat.st_mode))
    {
        std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << _httpRequest.getPath() << std::endl;
        // Obtener configuración temporal
        TempRouteConfig config = getTempRouteConfig(_httpRequest.getPath()); // TODO: Rehacer
        std::cout << "[DEBUG] Se pide servir un directorio. Entrando en AUTOINDEX" << std::endl;

        // Usar Autoindex para manejar el directorio
        Autoindex::handleDirectory(
            this,                   // Puntero a este Client
            fullPath,               // Ruta en disco
            _httpRequest.getPath(), // URL solicitada
            config.autoindex,       // ¿Autoindex activado?
            config.defaultFile      // Archivo por defecto (ej: "index.html")
        );
        return true; // Siempre hay respuesta lista, aunque handledirectory devuelva false, tenemos que devolver true para que siga el ciclo.
    }

    // Servir archivo estático (configura la respuesta tanto de éxito como de error)
    serveStaticFile(fullPath);
    return true; // Siempre hay respuesta HTTP
}

/*
handleGet() no “procesa la request entera”, solo decide:

“¿Esta request se resuelve con un archivo/directorio
o con otra cosa (CGI, error, etc.)?”

Por eso:
    Filesystem (si es archivo o directorio)→ solo PATH
    CGI → PATH + QUERY

*/

bool Client::handleHead()
{
    bool headMethod = handleGet(); // reutiliza GET COMPLETO
    _httpResponse.setBody("");     // elimina body
    return headMethod;
}
/*
HEAD es lo mismo que get, pero no debe mostrar el body, por eso lo eliminamos
*/

bool Client::handlePost()
{
    // 1. Por ahora, solo permitimos POST en /upload
    if (_httpRequest.getPath() != "/upload")
    {
        _httpResponse.setErrorResponse(403); // Forbidden
        applyConnectionHeader();
        return true; // Respuesta lista, no error fatal
    }
    std::cout << "[DEBUG] POST path: " << _httpRequest.getPath() << std::endl;

    // 2. Transfer-Encoding: chunked
    //  TODO: Si el cliente envía chunked encoding → no soportado. REVISAR SI HAY QUE ACEPTQRLO O RECHAZAR CON 411/400 o que
    if (_httpRequest.isChunked())
    {
        _httpResponse.setStatus(501, "Not Implemented");
        _httpResponse.setHeader("Content-Type", "text/html");
        _httpResponse.setBody("<html><body><h1>501 Not Implemented</h1>"
                              "<p>Chunked uploads are not supported.</p>"
                              "</body></html>");
        applyConnectionHeader();
        return true;
    }

    // 4. Crear/verificar directorio de uploads si no existe
    std::string uploadDir = std::string(WWW_ROOT) + "/uploads";

    struct stat st;
    if (stat(uploadDir.c_str(), &st) == -1) // si falla la carpeta no existe o algo raro
    {
        // NO existe → solo lo creamos si realmente es ENOENT
        if (errno == ENOENT)
        {
            if (mkdir(uploadDir.c_str(), 0755) == -1) // Permisos 0755 → lectura + escritura para owner, lectura para otros. Si falla, damos error del servidor
            {
                _httpResponse.setErrorResponse(500);
                applyConnectionHeader();
                return true;
            }
        }
        else
        {
            // stat falló por otra razón → error
            _httpResponse.setErrorResponse(500);
            applyConnectionHeader();
            return true;
        }
    }
    else // stat no falla
    {
        // Existe: asegurar que es un directorio
        if (!S_ISDIR(st.st_mode))
        {
            _httpResponse.setErrorResponse(500);
            applyConnectionHeader();
            return true;
        }
    }

    // Comprobar permisos de escritura explícitamente
    if (access(uploadDir.c_str(), W_OK) != 0)
    {
        _httpResponse.setErrorResponse(500);
        applyConnectionHeader();
        return true;
    }

    // 5. Generar nombre de archivo único (muy baja colisión)
    std::ostringstream ss;
    time_t now = time(NULL);
    pid_t pid = getpid();
    int rnd = rand();

    ss << "upload_" << now << "_" << pid << "_" << rnd << ".dat";

    std::string filename = ss.str();
    std::string filepath = uploadDir + "/" + filename;

    // 6. Escribir archivo de forma robusta (open/write/fsync)
    // O_CREAT | O_EXCL garantiza que no se sobrescribe un archivo existente.

    int fd = open(filepath.c_str(),
                  O_WRONLY | O_CREAT | O_EXCL,
                  0644);
    if (fd == -1)
    {
        _httpResponse.setErrorResponse(500);
        applyConnectionHeader();
        return true;
    }

    const std::string &body = _httpRequest.getBody(); // Referencia para evitar copia
    const char *buf = body.data();                    // aunque body ya tiene los datos, no puedes usarlos directamente con write, porque trabaja con punteros a bytes
    size_t total = body.size();
    size_t written = 0; // para saber cuánto llevamos escrito

    while (written < total) // write() puede escribir menos bytes de los pedidos, por eso hacemos bucle
    {
        ssize_t ret = write(fd, buf + written, total - written);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue; // Si una señal interrumpe write → repetir.

            // Error real → eliminar archivo incompleto
            close(fd);
            unlink(filepath.c_str()); // Limpiamos el archivo corrupto con unlink()
            _httpResponse.setErrorResponse(500);
            applyConnectionHeader();
            return true;
        }
        written += static_cast<size_t>(ret);
    }

    fsync(fd); // Fuerza la escritura del archivo físicamente a disco.
               // Cuando haces write():
               // Datos van al buffer del kernel (en RAM)
               // Kernel decide cuándo escribirlos realmente en disco
               // Podría tardar segundos si el sistema está ocupado
    close(fd);

    // 7. Preparar respuesta HTTP 201 Created
    _httpResponse.setStatus(201, "Created");
    _httpResponse.setHeader("Content-Type", "text/html");
    _httpResponse.setHeader("Location", "/uploads/" + filename);

    std::ostringstream html;
    html << "<html><body>"
         << "<h1>Upload successful</h1>"
         << "<p>Saved as: " << filename << " (" << total << " bytes)</p>"
         << "</body></html>";

    std::string htmlBody = html.str();
    _httpResponse.setBody(htmlBody);

    std::ostringstream len;
    len << htmlBody.size();
    _httpResponse.setHeader("Content-Length", len.str());

    applyConnectionHeader();

    std::cout << "[POST] Upload OK => " << filename
              << " (" << total << " bytes)" << std::endl;

    return true;
}

/*
TODO: en el primer párrafo, revisar si quiero quizás usar 404 en vez de 403 o permitir subrutas

*/

// Nunca se usa query en el delete de archivos. Lo ignoramos completamente
bool Client::handleDelete()
{
    // 1. Obtener path crudo de la request
    std::string rawPath = _httpRequest.getPath();

    // Decodificar
    std::string decoded = getDecodedPath();

    // Sanitizar
    std::string sanitized = sanitizePath(decoded);

    // Construir ruta real
    std::string fullPath = buildFullPath(sanitized);

    // 2. Verificar que no es una ruta prohibida
    //    (path traversal, etc. - ya gestionado por sanitizePath)

    if (fullPath == "__FORBIDDEN__")
    {
        _httpResponse.setErrorResponse(403); // Forbidden
        applyConnectionHeader();
        return true;
    }
    std::cout << "[DEBUG] DELETE fullPath: " << fullPath << std::endl;

    // 3. Verificar que el archivo existe
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0)
    {
        if (errno == ENOENT)
        // ❌ El archivo no existe

        {
            _httpResponse.setErrorResponse(404); // Not Found
        }
        else if (errno == EACCES)
        // ❌ No tenemos permiso para siquiera ver si existe

        {
            _httpResponse.setErrorResponse(403); // Forbidden - no permisos para ver
        }
        else
        // ❌ Otro error (dispositivo, enlace roto, etc.)

        {
            _httpResponse.setErrorResponse(500); // Internal Server Error
        }
        applyConnectionHeader();
        return true;
    }

    // 4. No permitir borrar directorios (seguridad)
    if (S_ISDIR(fileStat.st_mode))
    {
        _httpResponse.setErrorResponse(403); // Forbidden
        applyConnectionHeader();
        return true;
    }

    // 5. Verificar permisos de escritura en el directorio padre
    //    (stat() puede funcionar pero remove() fallar por permisos)

    std::string parentDir = fullPath.substr(0, fullPath.find_last_of('/'));
    if (parentDir.empty())
        parentDir = ".";

    if (access(parentDir.c_str(), W_OK) != 0)
    {
        _httpResponse.setErrorResponse(403); // Forbidden - no permisos para borrar
        applyConnectionHeader();
        return true;
    }

    // 6. Intentar borrar el archivo
    if (std::remove(fullPath.c_str()) != 0) // si falla
    {
        if (errno == EACCES || errno == EPERM)
        {
            _httpResponse.setErrorResponse(403); // Forbidden - permisos
        }
        else if (errno == EISDIR)
        {
            _httpResponse.setErrorResponse(403); // Forbidden - es directorio
                                                 // Esto no debería pasar porque ya verificamos S_ISDIR,
            // pero por si acaso (symlinks a directorios)
        }
        else
        {
            _httpResponse.setErrorResponse(500); // Internal Server Error
        }
        applyConnectionHeader();
        return true;
    }

    // 7. Respuesta exitosa (204 No Content - estándar para DELETE exitoso)
    _httpResponse.setStatus(204, "No Content");
    _httpResponse.setHeader("Content-Length", "0"); // opcional pero recomendable
    applyConnectionHeader();
    // NOTA: 204 No Content no debe tener body según RFC

    std::cout << "[DELETE] Archivo borrado: " << fullPath << std::endl;
    return true;
}

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
        resetForNextRequest();
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

time_t Client::getLastActivity() const
{
    return _lastActivity;
}

bool Client::isTimedOut(time_t now, int timeoutSec) const
{
    // Si por alguna razón 'now' es anterior o igual a la última actividad,
    // no consideramos que haya timeout.
    if (now <= _lastActivity)
        return false;

    // Tiempo transcurrido desde la última actividad
    time_t elapsed = now - _lastActivity;

    // Si ha pasado más que el límite, hay timeout
    return elapsed > timeoutSec;
}

void Client::resetForNextRequest()
{
    _httpRequest.reset();
    _httpResponse = HttpResponse();
    _requestComplete = false;
    _rawRequest.clear();
    _writeBuffer.clear();
    _writeOffset = 0;
}

// Helpers autoindex

bool Client::sendHtmlResponse(const std::string &html)
{
    _httpResponse.setStatus(200, "OK");
    _httpResponse.setHeader("Content-Type", "text/html; charset=utf-8");
    // Es importante el charset para caracteres especiales.
    // TODO: hay que poner el mime no? no solo sera tipo html

    std::ostringstream len;
    len << html.size();
    _httpResponse.setHeader("Content-Length", len.str()); // buscar cuántos bytes de HTML enviar

    applyConnectionHeader();
    _httpResponse.setBody(html); // Guardamos el HTML en el cuerpo de la respuesta

    return true;
} // TODO:revisar lo de mime y tambien si en serve static file mejor llamar a esto

bool Client::sendError(int errorCode)
{
    _httpResponse.setErrorResponse(errorCode);
    applyConnectionHeader();
    return true;
}

// Configuración temporal (reemplazar cuando se termine config)
Client::TempRouteConfig Client::getTempRouteConfig(const std::string &path)
{
    TempRouteConfig config;
    config.autoindex = false; // Por defecto OFF
    config.defaultFile = "index.html";
    std::cout << "hay autoindex????: " << config.autoindex << std::endl;

    // Configurar rutas ESPECÍFICAS con autoindex ON
    // Aquí pondrías tu lógica temporal
    if (path.find("/tests/files") == 0 ||
        path.find("/tests/public") == 0 ||
        path.find("/tests/uploads") == 0)
    {
        config.autoindex = true;
        std::cout << "[DEBUG] Autoindex ON para: " << path << std::endl;
        // TODO: Acabar de entender el criterio para poner autoindex ON
    }
    std::cout << "hay autoindex????: " << config.autoindex << std::endl;
    return config;
}