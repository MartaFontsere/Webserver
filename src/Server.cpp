#include "Server.hpp"
#include <iostream> // para imprimir mensajes
#include <cstring>  // para memset, strerror, strlen...
#include <unistd.h> // para close(), read, write
#include <fcntl.h>  // para fcntl() → modo no bloqueante
// CUAL DE LAS DOS? #include <netinet/in.h> // sockaddr_in, htons, etc.
#include <arpa/inet.h>  // para sockaddr_in, htons, INADDR_ANY
#include <sys/socket.h> // para socket(), bind(), listen()

// Constructor: guarda el puerto que usaremos
Server::Server(const std::string &port) : _port(port), _serverFd(-1)
{
}

// Destructor: si el socket está abierto, lo cerramos
Server::~Server()
{
    if (_serverFd != -1)
        close(_serverFd);
}

/*
Un socket es un descriptor de archivo especial (como un int) que representa una conexión de red.

En el constructor, solo guardamos el puerto (aún no creamos el socket).
_serverFd se inicializa con -1 para indicar “no hay socket abierto todavía”.

En el destructor, comprobamos si el socket se creó (_serverFd != -1), y lo cerramos para liberar recursos del sistema.
    Los recursos (como sockets) deben liberarse automáticamente cuando el objeto se destruye.
*/

bool Server::init()
{
    // Creamos y asociamos el socket al puerto
    _serverFd = createAndBind(_port.c_str()); //_port.c_str() significa que le estás pasando el puerto como cadena de caracteres erminado en \0, es decir, un const char* al estilo C.
    if (_serverFd == -1)
    {
        std::cerr << "❌ Error: no se pudo crear el socket." << std::endl;
        return false;
    }

    // Lo ponemos en modo no bloqueante
    if (setNonBlocking(_serverFd) == -1)
    {
        std::cerr << "❌ Error: no se pudo poner el socket en modo no bloqueante." << std::endl;
        return false;
    }

    // Empezamos a escuchar
    if (listen(_serverFd, SOMAXCONN) == -1)
    {
        std::cerr << "❌ Error en listen()." << std::endl;
        return false;
    }

    std::cout << "✅ Servidor escuchando en el puerto " << _port << std::endl;
    return true;
}

/*
Esta es la función principal de inicialización del servidor.

Llama a createAndBind() → que crea el socket y lo asocia a una dirección IP y puerto.

Llama a setNonBlocking() → para que las llamadas accept(), recv(), send() no bloqueen.

Luego hace listen() → el servidor empieza a “escuchar” nuevas conexiones entrantes. Este paso convierte el socket en servidor pasivo. El listen le dice al Kernel que ese socket ya no va a iniciar conexiones (deja de ser cliente), ahora va a escucharlas y aceptarlas (socket de escucha).
    listen(server_fd, backlog); -> El segundo argumento (backlog) define el número máximo de conexiones pendientes que el kernel puede mantener en cola antes de que tú las aceptes.
        SOMAXCONN es una constante del sistema (normalmente 128 o más).

        Si 150 clientes intentan conectarse al mismo tiempo y tú solo has aceptado 100, los 50 restantes esperan en esa cola.

        Si se llena, el resto recibirán un error tipo connection refused.

    listen() no acepta conexiones.
    Solo prepara al kernel para recibirlas y meterlas en cola.
    accept() es la que realmente crea un nuevo socket para hablar con cada cliente.

Si cualquiera de estas partes falla, devuelve false.

🧠 Concepto importante:
En red, “bloquear” significa que el programa se detiene esperando algo (por ejemplo, un cliente que nunca responde).
Si todo fuera bloqueante, solo podrías atender a un cliente a la vez — por eso usamos modo no bloqueante.
*/

int Server::createAndBind(const char *port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        std::cerr << "Error creando socket: " << strerror(errno) << std::endl;
        return -1;
    }

    // Reusar la dirección inmediatamente si el servidor se reinicia
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;              // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;      // Escucha en todas las interfaces
    addr.sin_port = htons(std::atoi(port)); // Puerto → formato de red

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Error en bind(): " << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/*
En nuestro servidor, necesitamos un socket que:

    Escuche conexiones en un puerto concreto (por ejemplo, 8080).

    Esté asociado a una dirección IP (normalmente 0.0.0.0, o sea “todas las interfaces locales”).

    Pueda aceptar clientes que intenten conectarse a él.

    👉 La función createAndBind() se encarga de crear ese socket y vincularlo (bind) al puerto donde escuchará.


Por qué recibe un const char *port en lugar de std::string
    Esto es simplemente por compatibilidad con funciones de C antiguas.
    socket(), bind(), htons() y atoi() son funciones de la librería C, no de C++.

    atoi() (convertir cadena a número) espera un const char *.

    Así que cuando en el constructor del servidor hacemos _serverFd = createAndBind(_port.c_str());
    ... lo que estamos haciendo es convertir el std::string a const char* para que lo pueda usar atoi().



La función crea y configura el socket para escuchar conexiones:

socket(AF_INET, SOCK_STREAM, 0)
→ Crea un socket TCP (orientado a conexión).
    AF_INET = familia de direcciones IPv4.
    SOCK_STREAM = tipo de socket orientado a conexión (TCP).
    0: protocolo por defecto (TCP).
Si devuelve -1, algo falló (no se pudo reservar el socket).

setsockopt(... SO_REUSEADDR ...)
→ Permite reiniciar el servidor sin esperar a que el puerto se libere (evita “Address already in use”).
    Esta parte permite reutilizar el puerto inmediatamente si reinicias el servidor.
    Sin esto, si paras y arrancas rápido, el SO podría decir:

    “Address already in use” 😩

    Porque el puerto sigue en estado TIME_WAIT unos segundos tras cerrar el socket.

    💡 SO_REUSEADDR le dice al kernel:

    “Tranquilo, sé lo que hago, déjame reutilizar el puerto enseguida”.

    ***Pero porque deberia reiniciarse el servidor?
        Este punto (el de setsockopt(... SO_REUSEADDR ...)) suele parecer mágico o innecesario al principio… pero en realidad tiene que ver con cómo funciona el sistema operativo, no solo con tu código.

        🧩 1️⃣ Qué pasa cuando tu servidor arranca

        Cuando haces esto:

        int sockfd = socket(...);
        bind(sockfd, ...);
        listen(sockfd, ...);


        El sistema operativo (Linux, macOS, etc.) reserva el puerto que le has indicado.
        Por ejemplo, si pides el puerto 8080, el sistema dice:

        “Vale, el proceso X está usando el puerto 8080, nadie más puede usarlo mientras siga abierto.”

        Así evita conflictos (dos programas intentando escuchar en el mismo puerto).

        🧩 2️⃣ Qué pasa cuando cierras el servidor

        Cuando terminas tu programa (o lo paras con Ctrl+C), en teoría ese socket debería cerrarse y liberar el puerto.
        Pero el sistema operativo no lo libera de inmediato ⚠️

        ¿Por qué?
        Porque en una conexión TCP, hay un mecanismo de seguridad para asegurarse de que no se pierdan mensajes pendientes.
        Cuando cierras el socket, las conexiones que tenía abiertas entran en un estado llamado TIME_WAIT.

        🔎 En ese estado:

        El puerto sigue “reservado” durante unos segundos (a veces 30–60).

        Aunque tu proceso ya terminó, el kernel mantiene esa reserva temporal.

        El resultado es que si intentas reiniciar el servidor inmediatamente (por ejemplo, compilas y lo vuelves a ejecutar enseguida), te salta este error:

        Error: bind() failed
        Address already in use


        🧩 3️⃣ Qué significa “reiniciar el servidor”

        No es que tu código se “reinicie solo”.
        Reiniciar significa algo como:

        Tú paras el programa (Ctrl+C, o matas el proceso).

        Lo vuelves a ejecutar enseguida (por ejemplo, porque has recompilado para probar algo nuevo).

        Ejemplo práctico:

        $ ./webserv
        # Servidor escuchando en el puerto 8080...

        # Lo detienes:
        ^C   # (Ctrl+C)

        # Lo vuelves a ejecutar:
        $ ./webserv
        Error: bind() failed: Address already in use


        💥 Este error se da porque el sistema operativo aún tiene el puerto 8080 bloqueado en TIME_WAIT.

        🧩 4️⃣ Qué hace setsockopt(SO_REUSEADDR)

        Esa llamada es una configuración opcional del socket, y su función es decirle al sistema:

        “Tranquilo, quiero reutilizar el puerto incluso si está en TIME_WAIT.”

        Es decir:
        ✅ Permite volver a hacer bind() sobre el mismo puerto aunque el SO crea que “aún está en uso” por una conexión previa del mismo programa.

        No afecta a la seguridad ni al funcionamiento normal.
        Solo acelera el ciclo de desarrollo y evita que tengas que esperar medio minuto cada vez que haces un cambio en el código.


        Si no estoy en TIME_WAIT, ¿para qué quiero SO_REUSEADDR? ¿No hace nada, o incluso puede fastidiar algo?”

            👉 No, no molesta, y sí conviene dejarla siempre.
            En la mayoría de casos no cambia nada cuando el puerto está libre, y solo actúa cuando lo necesitas (cuando está ocupado en TIME_WAIT).

            El sistema operativo simplemente ignora la opción porque no tiene nada que “reutilizar”.
            El bind() funciona igual que siempre, sin efectos secundarios.

            ✅ Así que no pasa absolutamente nada diferente respecto a no haber puesto la línea.

Se llena la estructura sockaddr_in con:

    sin_family: AF_INET → familia IPv4

    sin_addr.s_addr: INADDR_ANY → escucha en cualquier IP local, es decir, escuchará en 127.0.0.1, 192.168.x.x, etc.

    sin_port: htons() → convierte el número de puerto al formato de red (big endian).

bind() → asocia el socket al puerto del sistema operativo.

💡 Si bind() falla, puede ser porque ya hay otro programa usando ese puerto.


*** Explicación más en profundidad:

sockaddr_in es una estructura de C (no de C++) que describe una dirección de red IPv4.
Está definida en el archivo: #include <netinet/in.h>
Su definición simplificada es más o menos así:
    struct sockaddr_in {
        sa_family_t    sin_family; // Familia de direcciones (AF_INET)
        in_port_t      sin_port;   // Puerto (en formato network byte order)
        struct in_addr sin_addr;   // Dirección IP (también en formato network byte order)
        unsigned char  sin_zero[8]; // Relleno (no se usa, pero mantiene el tamaño)
    };


🔹 Qué representa
    Piensa que un socket es como un enchufe universal, pero para que el sistema operativo sepa a qué puerto y a qué IP quieres enchufarte, tienes que darle una dirección completa.

    🧠 Así que sockaddr_in ≈ “tarjeta con la dirección postal del servidor”:
        sin_family = tipo de dirección (por ejemplo, IPv4 o IPv6).
        sin_port = puerto donde escuchas (ej. 8080).
        sin_addr = IP donde quieres escuchar (ej. 127.0.0.1 o 0.0.0.0).

🔹 Por qué la necesitamos
    Las funciones del sistema (como bind(), connect(), sendto(), etc.) son muy antiguas, vienen del mundo C, y todas esperan recibir un puntero genérico a una dirección:
        struct sockaddr*

    Pero nosotros usamos la versión más específica:
        struct sockaddr_in

    Así que cuando la pasamos a una función, tenemos que hacer un cast:
        (struct sockaddr*)&addr
                ***Explicación: struct sockaddr_in addr; crea una estructura sockaddr_in, que sirve para guardar la dirección IP y el puerto cuando trabajas con IPv4.
                Tu variable addr es un sockaddr_in, pero la función espera un sockaddr*. Entonces necesitamos hacer un casteo
                Esto significa:
                    &addr → dirección de memoria de la variable addr (un puntero a sockaddr_in)
                    (struct sockaddr*) → le decimos al compilador:
                        “Tranquilo, trata este puntero como si apuntara a una sockaddr genérica.”
                No cambia los datos en memoria, solo la forma en que los interpretamos.


    Esto es porque la función no sabe si le estás pasando una dirección IPv4 (sockaddr_in), IPv6 (sockaddr_in6), o Unix domain socket (sockaddr_un).
    El cast solo le dice: “tranquilo, es del tipo genérico sockaddr*, pero realmente contiene una dirección IPv4”.


    🧠 Ejemplo: el bloque real de código
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; // IPv4
        addr.sin_addr.s_addr = INADDR_ANY; // Escucha en todas las interfaces
        addr.sin_port = htons(port); // Puerto (convertido a formato de red)

    🧩 Explicación línea a línea
        1️⃣ addr.sin_family = AF_INET;

        Le decimos que es una dirección IPv4 (no IPv6).
        Este valor (AF_INET) está definido en <sys/socket.h>.

        💡 Si usaras IPv6, pondrías AF_INET6.

        2️⃣ addr.sin_addr.s_addr = INADDR_ANY;

        Esto significa:
            “Escucha en todas las interfaces disponibles.”

        Si tu máquina tiene varias IPs (por ejemplo, una interna y otra externa), con INADDR_ANY el servidor aceptará conexiones desde cualquiera.

        💬 Alternativas:
            Si quisieras escuchar solo en localhost, pondrías:
                addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        Si quisieras una IP concreta, también podrías convertirla con inet_addr("192.168.1.42").

        3️⃣ addr.sin_port = htons(port);

        port aquí es el número de puerto que tú decides (por ejemplo, 8080).

        Pero —muy importante— el sistema operativo no guarda los números igual que tu CPU.
        Las CPUs pueden ser little endian o big endian, y eso afecta al orden de los bytes.

        💡 Ejemplo:
            Puerto 8080 = 0x1F90

        En memoria en un Intel (little endian) se guarda como 90 1F.
        En red (network order, big endian) debe ser 1F 90.
        Por eso usamos:
            htons()  // host to network short

        Para convertir automáticamente al formato correcto antes de pasar el valor al sistema.

    4️⃣ ¿Y el bind()?

        Una vez has rellenado addr, haces:
            bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))

        Esto le dice al sistema operativo:
            “Asocia mi socket (identificado por sockfd) con esta dirección IP y este puerto.”

        Sin esto, el socket no está “anclado” a ninguna dirección, y el sistema no sabría qué conexiones deben llegarle.

*/

int Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/*
Por defecto, un socket en Linux es bloqueante.
🚫 Qué significa “bloqueante”
    Un socket bloqueante detiene la ejecución del programa hasta que la operación termina.

    Por ejemplo:
        int client_fd = accept(server_fd, ...);

    👉 Si no hay ningún cliente intentando conectarse, esta línea se queda esperando indefinidamente.

    Lo mismo ocurre con:
        recv() → espera hasta que haya datos.

        send() → espera si el buffer está lleno.

    Esto está bien si tu programa solo maneja una conexión a la vez.
    Pero si estás escribiendo un servidor multipropósito, como tu webserv, eso sería un desastre: mientras una conexión está “esperando”, las demás se quedan congeladas.

⚙️ Qué hace “modo no bloqueante”

    Cuando el socket está en modo no bloqueante, esas funciones (accept, recv, send, etc.) no bloquean el flujo del programa.

        Si no hay nada que aceptar, accept() devuelve -1 e errno se pone en EAGAIN o EWOULDBLOCK.

        Si no hay datos disponibles en recv(), pasa lo mismo.

        Tú puedes seguir ejecutando el resto de tu código (por ejemplo, atender otros sockets).

    Esto es esencial para usar poll, select, o epoll — mecanismos que te dicen cuándo un socket está listo para leer o escribir, sin quedarte bloqueado.




Esta funcion hace que el socket no bloquee.

fcntl(fd, F_GETFL, 0) obtiene las flags actuales del descriptor fd.

fcntl(fd, F_SETFL, flags | O_NONBLOCK) activa la flag O_NONBLOCK.
    No borra los anteriores.
    Solo indica que el socket ya no bloqueará el flujo.

Así, si haces accept() y no hay clientes esperando, la llamada no se queda congelada, sino que devuelve inmediatamente con un error controlable (EAGAIN o EWOULDBLOCK).

Esto será esencial más adelante cuando usemos poll().
*/

/*
Qué tienes hasta ahora

    Has creado un objeto Server capaz de:

    Crear un socket TCP.

    Asociarlo a un puerto.

    Escuchar conexiones sin bloquear.

    Cerrar todo ordenadamente al destruir el objeto.

➡️ Todavía no acepta clientes ni responde datos, pero ya es un servidor inicializado que escucha.
Lo siguiente será crear un main.cpp que lo use y añadir el bucle principal (aceptar conexiones y enviar un “Hello world”).*/

int Server::getServerFd() const
{
    return _serverFd;
}

void Server::run()
{
    while (true)
    {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientFd < 0)
        {
            // No hay conexión nueva (puede pasar si el socket es non-blocking)
            continue;
        }

        std::cout << "Nueva conexión aceptada!" << std::endl;

        // Mensaje HTTP de respuesta
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 12\r\n"
            "\r\n"
            "Hello world!";

        // Enviamos la respuesta al cliente
        send(clientFd, response, strlen(response), 0);

        // Cerrar la conexión con el cliente -> Osea cerramos el socket del cliente (ya terminamos con él)
        close(clientFd);
    }
}

/*
1️⃣ while(true)
    Este es el bucle principal del servidor, el loop infinito del servidor.

    Queremos que siga escuchando y respondiendo sin parar.

    El servidor siempre debe estar escuchando nuevas conexiones.
    En esta versión básica no tenemos “poll()” ni “threads”, así que el servidor trabaja con una conexión a la vez.

    En una versión más avanzada, usaremos poll() o select() para manejar muchos clientes a la vez,
    pero de momento este bucle es suficiente para entender el flujo básico.

    clientAddr es donde accept() va a guardar los datos del cliente que se conecta (su IP y puerto).

    clientLen le dice a accept() cuánto espacio tiene para escribir esos datos.
    Por eso lo inicializas con sizeof(clientAddr) — no porque clientAddr tenga datos, sino para que accept() sepa cuánto puede llenar (el tamaño de la estructura).


    sockaddr_in es una estructura de C definida en los headers del sistema, concretamente en:
        #include <netinet/in.h>

    Su propósito es representar una dirección IPv4 (IP + puerto)
    para cuando queremos conectar, escuchar o aceptar conexiones.
        C++ importa esos nombres como tipos propios.
        Así que ya no hace falta escribir struct cada vez.

        Por eso puedes escribir simplemente:

        sockaddr_in clientAddr;


        y es exactamente equivalente a:

        struct sockaddr_in clientAddr;



2️⃣ accept()
    int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);

    accept() bloquea (espera) hasta que un cliente intenta conectarse a nuestro puerto (por ejemplo, con curl http://localhost:8080).

    Cuando eso ocurre:

        accept() devuelve un nuevo descriptor (clientFd), distinto de _serverFd --> Crea un nuevo socket (el clientFd) exclusivo para hablar con ese cliente.
            Devuelve la dirección IP y el puerto del cliente en clientAddr.
            Así el socket _serverFd sigue escuchando nuevas conexiones,
            y el clientFd se usa solo para atender a esa persona concreta, es la “línea privada” con ese cliente concreto.

    🧠 Piensa: _serverFd es la “recepcionista”, clientFd es la conversación privada.

    if (clientFd < 0)
        accept() puede devolver -1 si no hay ninguna conexión pendiente todavía.
        No pasa nada: simplemente seguimos el bucle y volvemos a intentarlo.

        Esto es gracias a haber puesto el socket en modo no bloqueante (setNonBlocking). Hace que las operaciones como accept(), recv() o send() no se queden esperando (bloqueadas) si no hay nada que hacer.
        En vez de eso, devuelven -1 inmediatamente y ponen errno = EAGAIN o EWOULDBLOCK.
            …ese if detecta dos posibles casos:
                Un error real (por ejemplo, algo falló en el sistema).
                Que no había ninguna conexión lista (lo típico en modo no bloqueante → EAGAIN).

        De lo contrario, accept() se quedará esperando hasta que alguien se conecte.



3️⃣ response

    Aquí estamos construyendo una respuesta HTTP completa.

    HTTP/1.1 200 OK
    Content-Type: text/plain
    Content-Length: 12

    Hello world!


    🔸 Primera línea:
    El estado de la respuesta → “200 OK” significa que todo ha ido bien.

    🔸 Cabeceras (headers):
    Le dicen al cliente qué tipo de contenido enviamos (text/plain)
    y cuánto mide (12 bytes en este caso).

    🔸 Línea vacía (\r\n)
    Obligatoria: separa las cabeceras del contenido.

    🔸 Cuerpo:
    El texto real que queremos enviar → "Hello world!"


    En las respuestas y peticiones HTTP, las líneas no terminan solo con \n,
    sino con \r\n, que significa:

    \r → carriage return (retorno de carro, mueve el cursor al inicio de la línea)

    \n → line feed (salta a la línea siguiente)

    🔹 Viene del estándar original de los protocolos de red (influenciado por Telnet y por máquinas antiguas).
    🔹 Es una forma obligatoria en HTTP/1.0 y HTTP/1.1 para marcar los saltos de línea en los headers.


3️⃣ send()
    send(clientFd, response, strlen(response), 0);

    Envía datos al cliente usando el socket recién aceptado.

    Esto envía la cadena completa del mensaje HTTP.

    Aquí estamos mandando un mensaje HTTP completo, aunque muy simple:

        HTTP/1.1 200 OK
        Content-Type: text/plain

        Hello world!

    Eso permite que si abres el navegador en http://localhost:8080, o haces curl http://localhost:8080
    veas “Hello world!” directamente en pantalla 🎉


4️⃣ close()
    close(clientFd);

    Cierra el socket del cliente.

    Si no lo cierras, se quedarían conexiones “fantasma” abiertas (lo que provoca TIME_WAIT o fugas de descriptores).

    Esto significa que:
        Si el cliente (por ejemplo, un navegador) quiere pedir otro recurso,
        tendrá que abrir una nueva conexión TCP, o sea, un nuevo file descriptor.

    Pero ojo: HTTP tiene dos modos
        1. HTTP/1.0 (el que usamos aquí)

        → Cada petición usa una conexión nueva.
        → Servidor responde → se cierra el socket → fin.
        → El cliente abre otro si necesita más.

        2. HTTP/1.1 (Keep-Alive)

        → Permite mantener la conexión abierta y enviar varias peticiones seguidas.
        → Esto se indica con el header:
            Connection: keep-alive

        Entonces el servidor no cierra el socket hasta que:
            el cliente lo pida,
            o haya pasado un tiempo de inactividad.

    En tu caso (mini servidor inicial)
        Cerrar la conexión después de enviar la respuesta está perfecto ✅
        Más adelante, cuando tengas un bucle con poll() o select(),
        ya verás cómo mantener conexiones abiertas o detectar cuándo cerrarlas.

*/
