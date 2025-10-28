#include "Server.hpp"
#include <iostream>     // para imprimir mensajes
#include <cstring>      // para memset, strerror...
#include <unistd.h>     // para close()
#include <fcntl.h>      // para fcntl() → modo no bloqueante
#include <arpa/inet.h>  // para sockaddr_in, htons, INADDR_ANY
#include <sys/socket.h> // para socket(), bind(), listen()

// Constructor: guarda el puerto que usaremos
Server::Server(const std::string &port) : _port(port), _listenFd(-1)
{
}

// Destructor: si el socket está abierto, lo cerramos
Server::~Server()
{
    if (_listenFd != -1)
        close(_listenFd);
}

/*
Un socket es un descriptor de archivo especial (como un int) que representa una conexión de red.

En el constructor, solo guardamos el puerto (aún no creamos el socket).
_listenFd se inicializa con -1 para indicar “no hay socket abierto todavía”.

En el destructor, comprobamos si el socket se creó (_listenFd != -1), y lo cerramos para liberar recursos del sistema.
    Los recursos (como sockets) deben liberarse automáticamente cuando el objeto se destruye.
*/

bool Server::init()
{
    // Creamos y asociamos el socket al puerto
    _listenFd = createAndBind(_port.c_str()); //_port.c_str() significa que le estás pasando el puerto como cadena de caracteres erminado en \0, es decir, un const char* al estilo C.
    if (_listenFd == -1)
    {
        std::cerr << "❌ Error: no se pudo crear el socket." << std::endl;
        return false;
    }

    // Lo ponemos en modo no bloqueante
    if (setNonBlocking(_listenFd) == -1)
    {
        std::cerr << "❌ Error: no se pudo poner el socket en modo no bloqueante." << std::endl;
        return false;
    }

    // Empezamos a escuchar
    if (listen(_listenFd, 10) == -1)
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

Luego hace listen() → el servidor empieza a “escuchar” nuevas conexiones entrantes.

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

    Así que cuando en el constructor del servidor hacemos _listenFd = createAndBind(_port.c_str());
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
*/