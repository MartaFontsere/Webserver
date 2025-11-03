#pragma once

#include <string> //la clase usará std::string (para guardar el puerto, por ejemplo).
#include "Client.hpp"
#include <vector>
#include <map>

class Server
{
private:
    std::string _port; // el file descriptor del socket de escucha (el que usaremos con listen() y accept())
    int _serverFd;
    std::vector<struct pollfd> _pollFds;  // lista de FDs (sockets) a vigilar (server + clients)
    std::map<int, Client *> _clientsByFd; // map fd -> Client*

    int createAndBind(const char *port);
    int setNonBlocking(int fd);
    void acceptNewClient();
    void handleClientEvent(int fd);
    void cleanupClosedClients();

public:
    Server(const std::string &port); // puerto a escuchar
    ~Server();

    bool init(); // crea y prepara el socket (bind + listen + non-blocking)
    int getServerFd() const;
    void run();
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

    * int getServerFd() const → devuelve el file descriptor del socket principal, por si otro componente necesita acceder a él.

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
    _listenFd termina siendo el “enchufe” del servidor, ya listo para recibir conexiones.

    _port: el puerto en el que escuchamos (por ejemplo "8080"). Guardarlo como string facilita las llamadas a funciones del sistema que lo esperan así.
    Significa que tu puerto está guardado como texto, no como número.
        Esto es útil porque muchas veces los parámetros vienen de la línea de comandos: "8080"

        O de un archivo de configuración: "3000"

        Pero bind() y el resto de funciones de sockets necesitan un número entero, no un string.
*/

/*
std::map<int, Client> vs std::map<int, Client*>

Las dos opciones son posibles, pero cada una tiene implicaciones distintas 👇

✅ Opción 1 — std::map<int, Client>
std::map<int, Client> clients;


👉 Aquí cada Client se guarda directamente dentro del mapa, como un objeto completo.
Ventajas:

Gestión automática de memoria (no hay new ni delete).

Más seguro.

Desventajas:

Si necesitas mantener punteros o referencias estables a los Client, puede complicarse, porque el objeto puede moverse internamente si haces inserciones/borrados.

Copiar objetos Client puede ser costoso (si son grandes).

✅ Opción 2 — std::map<int, Client*>
std::map<int, Client*> clients;


👉 Aquí el mapa guarda punteros a objetos Client, no los objetos en sí.

Ventajas:

Puedes crear los clientes dinámicamente (new Client(fd)) y controlar cuándo se destruyen.

El puntero siempre es estable (no cambia aunque el mapa se modifique).

Desventajas:

Tienes que liberar manualmente la memoria (delete clientPtr) o usar punteros inteligentes (std::unique_ptr).

Si olvidas liberar, generas fugas de memoria.

🧭 En tu webserver (proyecto 42)

Normalmente se usa:

std::map<int, Client*> _clients;


porque:

cada cliente se asocia a un socket fd (el int),

y el servidor crea un nuevo Client dinámicamente cuando llega una conexión:

_clients[newFd] = new Client(newFd);


luego, cuando el cliente se desconecta:

delete _clients[fd];
_clients.erase(fd);


De este modo, cada cliente tiene su propio objeto con su socket, buffer, estado, etc.
 */