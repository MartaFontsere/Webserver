#pragma once

#include "config/ServerConfig.hpp"
#include "network/ClientConnection.hpp"
#include "network/PollManager.hpp"
#include "network/ServerSocket.hpp"
#include <map>
#include <string>
#include <vector>

class Server {
private:
  std::vector<ServerConfig>
      _servConfigsList; // lista de configuraciones del servidor
  std::vector<ServerSocket *> _serverSockets; // lista de sockets
  PollManager _pollManager;                   // poll manager

  typedef std::vector<ServerConfig>
      ConfigVector; // alias para vector de configuraciones
  std::map<int, ConfigVector>
      _configsByServerFd; // mapeo de configuraciones por fd
  std::map<int, ClientConnection *> _clientsByFd; // mapeo de clientes por fd

  // ====== CGI Pipe Tracking ======
  // Maps CGI pipe FD -> client that owns it (for poll() lookup)
  std::map<int, ClientConnection *> _cgiPipeToClient;

  void acceptNewClient(int serverFd);
  void handleClientData(ClientConnection *client, size_t pollIndex);
  void handleClientWrite(ClientConnection *client, size_t pollIndex);
  void handleCGIPipe(int pipeFd,
                     ClientConnection *client); // NEW: CGI pipe handler
  void checkClientTimeout(ClientConnection *client, int fd, time_t now);
  void cleanupClosedClients();
  std::map<int, ConfigVector>
  groupConfigsByPort(); // agrupa configuraciones por puerto

public:
  Server(const std::vector<ServerConfig> &configs); // constructor
  ~Server();                                        // destructor

  bool init(); // crea y prepara el socket (bind + listen + non-blocking)
  void run();
};

/*
¿Por qué una clase Server?

Queremos organizar el código de manera que cada parte del servidor (networking,
HTTP, config...) esté aislada y clara.

Crear una clase Server que represente nuestro servidor como un objeto:

    * Tiene su estado interno (por ejemplo, su socket de escucha y su puerto).

    * Tiene métodos que realizan acciones (inicializar, aceptar conexiones,
etc). Esto hace que el código sea más limpio, mantenible y fácil de extender
(mañana podrás añadir más puertos, logs, poll, etc).

Explicación de cada método público:

    * Server(const std::string& port) → constructor: cuando creas el objeto, le
dices en qué puerto debe escuchar.

    * ~Server() → destructor: limpia al final (por ejemplo, cierra el socket).

    * bool init() → inicializa todo el sistema de escucha (crea socket, lo
enlaza, lo pone a escuchar). Devuelve true si todo salió bien, false si falló.

Explicación de cada método privado:
    ¿Por qué esto es privado?

    Son funciones internas, no deberían usarse fuera de la clase.
    Así protegemos el funcionamiento interno y solo exponemos la interfaz segura
(el init()).

    🔹 Qué hace cada una:

    createAndBind(const char* port) → crea un socket y lo asocia (bind) al
puerto. Devuelve el descriptor de archivo (int) o -1 si falla.

    setNonBlocking(int fd) → marca el descriptor como no bloqueante.
    Esto será esencial para que el servidor pueda atender a varios clientes sin
quedarse congelado.

    🔹 Variables privadas:

    _listenFd: el descriptor del socket que escucha conexiones entrantes.
    Piensa en él como “la oreja” del servidor: se queda esperando conexiones
nuevas. _listenFd termina siendo el “enchufe” del servidor, ya listo para
recibir conexiones.

    _port: el puerto en el que escuchamos (por ejemplo "8080"). Guardarlo como
string facilita las llamadas a funciones del sistema que lo esperan así.
    Significa que tu puerto está guardado como texto, no como número.
        Esto es útil porque muchas veces los parámetros vienen de la línea de
comandos: "8080"

        O de un archivo de configuración: "3000"

        Pero bind() y el resto de funciones de sockets necesitan un número
entero, no un string.
*/

/*
std::map<int, Client> vs std::map<int, Client*>

Las dos opciones son posibles, pero cada una tiene implicaciones distintas 👇

✅ Opción 1 — std::map<int, ClientConnection>
std::map<int, ClientConnection> clients;


👉 Aquí cada ClientConnection se guarda directamente dentro del mapa, como un
objeto completo. Ventajas:

Gestión automática de memoria (no hay new ni delete).

Más seguro.

Desventajas:

Si necesitas mantener punteros o referencias estables a los Client, puede
complicarse, porque el objeto puede moverse internamente si haces
inserciones/borrados.

Copiar objetos Client puede ser costoso (si son grandes).

✅ Opción 2 — std::map<int, ClientConnection*>
std::map<int, ClientConnection*> clients;


👉 Aquí el mapa guarda punteros a objetos ClientConnection, no los objetos en sí.

Ventajas:

Puedes crear los clientes dinámicamente (new ClientConnection(fd)) y controlar
cuándo se destruyen.

El puntero siempre es estable (no cambia aunque el mapa se modifique).

Desventajas:

Tienes que liberar manualmente la memoria (delete clientPtr) o usar punteros
inteligentes (std::unique_ptr).

Si olvidas liberar, generas fugas de memoria.

🧭 En tu webserver (proyecto 42)

Normalmente se usa:

std::map<int, ClientConnection*> _clients;


porque:

cada cliente se asocia a un socket fd (el int),

y el servidor crea un nuevo ClientConnection dinámicamente cuando llega una
conexión:

_clients[newFd] = new ClientConnection(newFd);


luego, cuando el cliente se desconecta:

delete _clients[fd];
_clients.erase(fd);


De este modo, cada cliente tiene su propio objeto con su socket, buffer, estado,
etc.
 */
