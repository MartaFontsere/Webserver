#pragma once

#include <string> //la clase usará std::string (para guardar el puerto, por ejemplo).

class Server
{
public:
    Server(const std::string &port); // puerto a escuchar
    ~Server();
    bool init(); // crea y prepara el socket (bind + listen + non-blocking)
    int getListenFd() const;

private:
    int createAndBind(const char *port);
    int setNonBlocking(int fd);

    int _listenFd;
    std::string _port;
};

/*
¿Por qué una clase Server?

Queremos organizar el código de manera que cada parte del servidor (networking, HTTP, config...) esté aislada y clara.

Crear una clase Server que represente nuestro servidor como un objeto:

    * Tiene su estado interno (por ejemplo, su socket de escucha y su puerto).

    * Tiene métodos que realizan acciones (inicializar, aceptar conexiones, etc). Esto hace que el código sea más limpio, mantenible y fácil de extender (mañana podrás añadir más puertos, logs, poll, etc).

Explicación de cada método público:

    * Server(const std::string& port) → constructor: cuando creas el objeto, le dices en qué puerto debe escuchar.

    * ~Server() → destructor: limpia al final (por ejemplo, cierra el socket).

    * bool init() → inicializa todo el sistema de escucha (crea socket, lo enlaza, lo pone a escuchar). Devuelve true si todo salió bien, false si falló.

    * int getListenFd() const → devuelve el file descriptor del socket principal, por si otro componente necesita acceder a él.

Explicación de cada método privado:
    ¿Por qué esto es privado?

    Son funciones internas, no deberían usarse fuera de la clase.
    Así protegemos el funcionamiento interno y solo exponemos la interfaz segura (el init()).

    🔹 Qué hace cada una:

    createAndBind(const char* port) → crea un socket y lo asocia (bind) al puerto.
    Devuelve el descriptor de archivo (int) o -1 si falla.

    setNonBlocking(int fd) → marca el descriptor como no bloqueante.
    Esto será esencial para que el servidor pueda atender a varios clientes sin quedarse congelado.

    🔹 Variables privadas:

    _listenFd: el descriptor del socket que escucha conexiones entrantes.
    Piensa en él como “la oreja” del servidor: se queda esperando conexiones nuevas.

    _port: el puerto en el que escuchamos (por ejemplo "8080"). Guardarlo como string facilita las llamadas a funciones del sistema que lo esperan así.
*/