#include "core/Server.hpp"
#include "cgi/CGIHandler.hpp" // For buildResponseFromCGIOutput
#include <algorithm>
#include <cstdio>       // para perror
#include <cstring>      // para memset, strerror, strlen...
#include <fcntl.h>      // para fcntl() → modo no bloqueante
#include <iostream>     // para imprimir mensajes
#include <netinet/in.h> // para sockaddr_in
#include <poll.h>
#include <sstream>
#include <sys/socket.h> // para socket(), bind(), listen()
#include <sys/wait.h>   // para waitpid, WNOHANG
#include <unistd.h>     // para close(), read, write

#include <csignal> // para sig_atomic_t

extern volatile sig_atomic_t
    g_running; // Variable global para controlar el bucle principal
               // y determinar si el servidor debe continuar
               // ejecutándose. Se define en main.cpp.

// Constructor: guarda las configuraciones (es capaz de manjar múltiples
// configuraciones de servidor virtual)
Server::Server(const std::vector<ServerConfig> &servConfigsList)
    : _servConfigsList(servConfigsList) {}
/*
El server tiene su propio vector de ServerConfig. Guardamos una copia de la
lista que nos pasan.

La referencia (&servConfigsList) solo se usa para evitar copiar dos veces
la lista de configuraciones.
*/

// Destructor: si el socket está abierto, lo cerramos
Server::~Server() {
  // Cerrar todos los clientes
  for (std::map<int, ClientConnection *>::iterator it = _clientsByFd.begin();
       it != _clientsByFd.end(); ++it) {
    if (it->second) {
      delete it->second; // libera la memoria del objeto Client y ademas llama
                         // al destructor de client para cerrar el fd.
    }
  }
  _clientsByFd.clear();

  // Cerrar sockets servidores
  for (size_t i = 0; i < _serverSockets.size(); ++i) {
    delete _serverSockets[i]; // esto llama al destructor de ServerSocket, que a
                              // su vez llama a closeSocket()
  }
  _serverSockets.clear();
}
/*
Un socket es un descriptor de archivo especial (como un int) que representa una
conexión de red.

En el constructor, solo guardamos el puerto (aún no creamos el socket).
_serverFd se inicializa con -1 para indicar “no hay socket abierto todavía”.

En el destructor, comprobamos si el socket se creó (_serverFd != -1), y lo
cerramos para liberar recursos del sistema. Los recursos (como sockets) deben
liberarse automáticamente cuando el objeto se destruye. Es la limpieza final: si
el servidor se destruye (programa acaba o objeto eliminado), hay que liberar los
recursos del sistema: cerrar sockets y liberar memoria de new Client(...).

El dueño del file descriptor (FD) debe ser el objeto Client.
El Server crea y destruye clientes, pero no cierra sockets directamente, solo
borra los objetos Client.

Entonces:
    ClientConnection se encarga de cerrar su propio _clientFd.
    Server solo llama delete it->second;.
    No debe llamar a close() sobre los FDs.

Cuando haces:
    delete client;
ocurre exactamente esto, en orden:
    Se llama al destructor del objeto ClientConnection.
    Es decir, se ejecuta ClientConnection::~ClientConnection().
    El objeto sigue existiendo completamente durante la ejecución del destructor
— puedes leer _clientFd, _closed, etc.

    Dentro del destructor, tú puedes:
        Cerrar el socket (close(_clientFd)),
        Imprimir mensajes,
        Cambiar flags (_closed = true),
        O liberar recursos adicionales (memoria, ficheros, etc).

    Cuando termina el destructor,
    el compilador libera la memoria ocupada por el objeto.
    Solo en ese momento el puntero client ya no apunta a memoria válida.

Qué pasa con el FD después de close()

    Cuando haces close(_clientFd):
        El descriptor de archivo (número entero en el kernel) se libera.
        El sistema operativo puede reutilizarlo más tarde para otro socket.
        Pero tu variable _clientFd sigue existiendo en el objeto Client (con el
mismo número) hasta que el destructor termina.

    Por eso es buena práctica hacer:
        close(_clientFd);
        _clientFd = -1;

Así evitas “doble cierre” accidental.

DUDA:
Si el flujo es:
    Detectas que un Client está cerrado.
    Haces delete client.
    El destructor cierra el fd (si no se cerró antes).
    El objeto desaparece.

Entonces sí: nadie más debería acceder a ese objeto ni a ese fd.
En ese flujo limpio y lineal, no haría falta ni poner closed = true ni clientFd
= -1.

⚠️ Pero en la práctica...
El problema es que, en servidores no bloqueantes y con múltiples pasos, no
siempre el flujo es tan lineal o “perfecto”.

Ejemplos reales:

1. El Client puede seguir referenciado
    Aunque hagas delete client, puede que en otro punto del loop o en otra
estructura aún haya punteros colgantes (por ejemplo, si tenías un
std::vector<Client*> y no limpiaste bien los iteradores). 👉 Si el fd se marca a
-1, evitas intentar usar un descriptor ya cerrado.
*/

/*
std::map<int, ClientConnection> vs std::map<int, ClientConnection*>

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

// Inicializa el servidor: crea socket, bind y listen
bool Server::init() {
  // Agrupamos las configuraciones por puerto
  std::map<int, ConfigVector> configsByPort = groupConfigsByPort();

  // Creamos un socket por cada puerto único y asociamos las configuraciones
  // correspondientes

  for (std::map<int, ConfigVector>::iterator it = configsByPort.begin();
       it != configsByPort.end(); ++it) {
    int port = it->first;
    ServerSocket *serverSocket = new ServerSocket(port);

    if (!serverSocket->init()) {
      std::cerr << "❌ Failed to initialize server socket on port " << port
                << std::endl;
      delete serverSocket;
      return false;
    }

    int fd = serverSocket->getFd();
    _serverSockets.push_back(serverSocket);
    _configsByServerFd[fd] = it->second;

    // Añadimos el socket del servidor al PollManager
    _pollManager.addFd(fd, POLLIN);

    std::cout << "🌐 Server listening on port " << port << " (fd: " << fd << ")"
              << std::endl;
  }

  return true;
}

// Agrupa las configuraciones por puerto
std::map<int, std::vector<ServerConfig> > Server::groupConfigsByPort() {
  std::map<int, ConfigVector> configsByPort;
  for (size_t i = 0; i < _servConfigsList.size(); ++i) {
    configsByPort[_servConfigsList[i].getListen()].push_back(
        _servConfigsList[i]);
  }
  return configsByPort;
}

/*
std::map<int, std::vector<ServerConfig> > configsByPort;
Esto significa:
puerto → lista de reglas que escuchan ahí

Ejemplo mental:

server { listen 8080; server_name a; }
server { listen 8080; server_name b; }
server { listen 9090; server_name c; }


Después de este bloque, el map queda así:

8080 → [ ServerConfig(a), ServerConfig(b) ]
9090 → [ ServerConfig(c) ]

*/

// createAndBind y setNonBlocking han sido movidos a ServerSocket.cpp

/*
En nuestro servidor, necesitamos un socket que:

    Escuche conexiones en un puerto concreto (por ejemplo, 8080).

    Esté asociado a una dirección IP (normalmente 0.0.0.0, o sea “todas las
interfaces locales”).

    Pueda aceptar clientes que intenten conectarse a él.

    👉 La función createAndBind() se encarga de crear ese socket y vincularlo
(bind) al puerto donde escuchará.

Por qué recibe un const char *port en lugar de std::string
    Esto es simplemente por compatibilidad con funciones de C antiguas.
    socket(), bind(), htons() y atoi() son funciones de la librería C, no de
C++.

    atoi() (convertir cadena a número) espera un const char *.

    Así que cuando en el constructor del servidor hacemos _serverFd =
createAndBind(_port.c_str());
    ... lo que estamos haciendo es convertir el std::string a const char* para
que lo pueda usar atoi().

La función crea y configura el socket para escuchar conexiones:
    Crear socket(), configurar SO_REUSEADDR, preparar sockaddr_in y bind() al
puerto solicitado. Devuelve el descriptor o -1 en error.

Explicacion línea por línea:

socket(AF_INET, SOCK_STREAM, 0)
→ Crea un socket TCP IPv4 (orientado a conexión).
    AF_INET = familia de direcciones IPv4.
    SOCK_STREAM = tipo de socket orientado a conexión (TCP).
    0: protocolo por defecto (TCP).
Si devuelve -1, algo falló (no se pudo reservar el socket).

setsockopt(... SO_REUSEADDR ...)
→ Permite reiniciar el servidor sin esperar a que el puerto se libere (evita
“Address already in use”). Esta parte permite reutilizar el puerto
inmediatamente si reinicias el servidor. Sin esto, si paras y arrancas rápido,
el SO podría decir:

    “Address already in use” 😩

    Porque el puerto sigue en estado TIME_WAIT unos segundos tras cerrar el
socket.

    💡 SO_REUSEADDR le dice al kernel:

    “Tranquilo, sé lo que hago, déjame reutilizar el puerto enseguida”.

    ***Pero porque deberia reiniciarse el servidor?
        Este punto (el de setsockopt(... SO_REUSEADDR ...)) suele parecer mágico
o innecesario al principio… pero en realidad tiene que ver con cómo funciona el
sistema operativo, no solo con tu código.

        🧩 1️⃣ Qué pasa cuando tu servidor arranca

        Cuando haces esto:

        int sockfd = socket(...);
        bind(sockfd, ...);
        listen(sockfd, ...);

        El sistema operativo (Linux, macOS, etc.) reserva el puerto que le has
indicado. Por ejemplo, si pides el puerto 8080, el sistema dice:

        “Vale, el proceso X está usando el puerto 8080, nadie más puede usarlo
mientras siga abierto.”

        Así evita conflictos (dos programas intentando escuchar en el mismo
puerto).

        🧩 2️⃣ Qué pasa cuando cierras el servidor

        Cuando terminas tu programa (o lo paras con Ctrl+C), en teoría ese
socket debería cerrarse y liberar el puerto. Pero el sistema operativo no lo
libera de inmediato ⚠️

        ¿Por qué?
        Porque en una conexión TCP, hay un mecanismo de seguridad para
asegurarse de que no se pierdan mensajes pendientes. Cuando cierras el socket,
las conexiones que tenía abiertas entran en un estado llamado TIME_WAIT.

        🔎 En ese estado:

        El puerto sigue “reservado” durante unos segundos (a veces 30–60).

        Aunque tu proceso ya terminó, el kernel mantiene esa reserva temporal.

        El resultado es que si intentas reiniciar el servidor inmediatamente
(por ejemplo, compilas y lo vuelves a ejecutar enseguida), te salta este error:

        Error: bind() failed
        Address already in use

        🧩 3️⃣ Qué significa “reiniciar el servidor”

        No es que tu código se “reinicie solo”.
        Reiniciar significa algo como:

        Tú paras el programa (Ctrl+C, o matas el proceso).

        Lo vuelves a ejecutar enseguida (por ejemplo, porque has recompilado
para probar algo nuevo).

        Ejemplo práctico:

        $ ./webserv
        # Servidor escuchando en el puerto 8080...

        # Lo detienes:
        ^C   # (Ctrl+C)

        # Lo vuelves a ejecutar:
        $ ./webserv
        Error: bind() failed: Address already in use

        💥 Este error se da porque el sistema operativo aún tiene el puerto 8080
bloqueado en TIME_WAIT.

        🧩 4️⃣ Qué hace setsockopt(SO_REUSEADDR)

        Esa llamada es una configuración opcional del socket, y su función es
decirle al sistema:

        “Tranquilo, quiero reutilizar el puerto incluso si está en TIME_WAIT.”

        Es decir:
        ✅ Permite volver a hacer bind() sobre el mismo puerto aunque el SO crea
que “aún está en uso” por una conexión previa del mismo programa.

        No afecta a la seguridad ni al funcionamiento normal.
        Solo acelera el ciclo de desarrollo y evita que tengas que esperar medio
minuto cada vez que haces un cambio en el código.

        Si no estoy en TIME_WAIT, ¿para qué quiero SO_REUSEADDR? ¿No hace nada,
o incluso puede fastidiar algo?”

            👉 No, no molesta, y sí conviene dejarla siempre.
            En la mayoría de casos no cambia nada cuando el puerto está libre, y
solo actúa cuando lo necesitas (cuando está ocupado en TIME_WAIT).

            El sistema operativo simplemente ignora la opción porque no tiene
nada que “reutilizar”. El bind() funciona igual que siempre, sin efectos
secundarios.

            ✅ Así que no pasa absolutamente nada diferente respecto a no haber
puesto la línea.

Se llena la estructura sockaddr_in con:

    sin_family: AF_INET → familia IPv4

    sin_addr.s_addr: INADDR_ANY → escucha en cualquier IP local, es decir,
escuchará en 127.0.0.1, 192.168.x.x, etc.

    sin_port: htons() → convierte el número de puerto al formato de red (big
endian).

bind() → asocia el socket al puerto del sistema operativo.

💡 Si bind() falla, puede ser porque ya hay otro programa usando ese puerto.

*** Explicación más en profundidad:

sockaddr_in es una estructura de C (no de C++) que describe una dirección de red
IPv4. Está definida en el archivo: #include <netinet/in.h> Su definición
simplificada es más o menos así: struct sockaddr_in { sa_family_t    sin_family;
// Familia de direcciones (AF_INET) in_port_t      sin_port;   // Puerto (en
formato network byte order) struct in_addr sin_addr;   // Dirección IP (también
en formato network byte order) unsigned char  sin_zero[8]; // Relleno (no se
usa, pero mantiene el tamaño)
    };

🔹 Qué representa
    Piensa que un socket es como un enchufe universal, pero para que el sistema
operativo sepa a qué puerto y a qué IP quieres enchufarte, tienes que darle una
dirección completa.

    🧠 Así que sockaddr_in ≈ “tarjeta con la dirección postal del servidor”:
        sin_family = tipo de dirección (por ejemplo, IPv4 o IPv6).
        sin_port = puerto donde escuchas (ej. 8080).
        sin_addr = IP donde quieres escuchar (ej. 127.0.0.1 o 0.0.0.0).

🔹 Por qué la necesitamos
    Las funciones del sistema (como bind(), connect(), sendto(), etc.) son muy
antiguas, vienen del mundo C, y todas esperan recibir un puntero genérico a una
dirección: struct sockaddr*

    Pero nosotros usamos la versión más específica:
        struct sockaddr_in

    Así que cuando la pasamos a una función, tenemos que hacer un cast:
        (struct sockaddr*)&addr
                ***Explicación: struct sockaddr_in addr; crea una estructura
sockaddr_in, que sirve para guardar la dirección IP y el puerto cuando trabajas
con IPv4. Tu variable addr es un sockaddr_in, pero la función espera un
sockaddr*. Entonces necesitamos hacer un casteo Esto significa: &addr →
dirección de memoria de la variable addr (un puntero a sockaddr_in) (struct
sockaddr*) → le decimos al compilador: “Tranquilo, trata este puntero como si
apuntara a una sockaddr genérica.” No cambia los datos en memoria, solo la forma
en que los interpretamos.

    Esto es porque la función no sabe si le estás pasando una dirección IPv4
(sockaddr_in), IPv6 (sockaddr_in6), o Unix domain socket (sockaddr_un). El cast
solo le dice: “tranquilo, es del tipo genérico sockaddr*, pero realmente
contiene una dirección IPv4”.

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

        Si tu máquina tiene varias IPs (por ejemplo, una interna y otra
externa), con INADDR_ANY el servidor aceptará conexiones desde cualquiera.

        💬 Alternativas:
            Si quisieras escuchar solo en localhost, pondrías:
                addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        Si quisieras una IP concreta, también podrías convertirla con
inet_addr("192.168.1.42").

        3️⃣ addr.sin_port = htons(port);

        port aquí es el número de puerto que tú decides (por ejemplo, 8080).

        Pero —muy importante— el sistema operativo no guarda los números igual
que tu CPU. Las CPUs pueden ser little endian o big endian, y eso afecta al
orden de los bytes.

        💡 Ejemplo:
            Puerto 8080 = 0x1F90

        En memoria en un Intel (little endian) se guarda como 90 1F.
        En red (network order, big endian) debe ser 1F 90.
        Por eso usamos:
            htons()  // host to network short

        Para convertir automáticamente al formato correcto antes de pasar el
valor al sistema.

    4️⃣ ¿Y el bind()?

        Una vez has rellenado addr, haces:
            bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))

        Esto le dice al sistema operativo:
            “Asocia mi socket (identificado por sockfd) con esta dirección IP y
este puerto.”

        Sin esto, el socket no está “anclado” a ninguna dirección, y el sistema
no sabría qué conexiones deben llegarle.

*/

/*
Por defecto, un socket en Linux es bloqueante.
🚫 Qué significa “bloqueante”
    Un socket bloqueante detiene la ejecución del programa hasta que la
operación termina.

    Por ejemplo:
        int client_fd = accept(server_fd, ...);

    👉 Si no hay ningún cliente intentando conectarse, esta línea se queda
esperando indefinidamente.

    Lo mismo ocurre con:
        recv() → espera hasta que haya datos.

        send() → espera si el buffer está lleno.

    Esto está bien si tu programa solo maneja una conexión a la vez.
    Pero si estás escribiendo un servidor multipropósito, como tu webserv, eso
sería un desastre: mientras una conexión está “esperando”, las demás se quedan
congeladas.

⚙️ Qué hace “modo no bloqueante”

    Cuando el socket está en modo no bloqueante, esas funciones (accept, recv,
send, etc.) no bloquean el flujo del programa.

        Si no hay nada que aceptar, accept() devuelve -1 e errno se pone en
EAGAIN o EWOULDBLOCK.

        Si no hay datos disponibles en recv(), pasa lo mismo.

        Tú puedes seguir ejecutando el resto de tu código (por ejemplo, atender
otros sockets).

    Esto es esencial para usar poll, select, o epoll — mecanismos que te dicen
cuándo un socket está listo para leer o escribir, sin quedarte bloqueado.

Esta funcion hace que el socket no bloquee.

fcntl(fd, F_GETFL, 0) obtiene las flags actuales del descriptor fd.

fcntl(fd, F_SETFL, flags | O_NONBLOCK) activa la flag O_NONBLOCK.
    No borra los anteriores. F_SETFL escribe los flags combinados con O_NONBLOCK
    Solo indica que el socket ya no bloqueará el flujo.
    Retorna el valor de fcntl (0 en éxito, -1 en fallo).

Así, si haces accept() y no hay clientes esperando, la llamada no se queda
congelada, sino que devuelve inmediatamente con un error controlable (EAGAIN o
EWOULDBLOCK). Osea, en modo non-blocking, accept(), recv() y send() no
bloquearán. En su lugar devolverán -1 y errno en EAGAIN/EWOULDBLOCK si no hay
datos/disponibilidad. poll() se usa para evitar llamadas en momentos con
posibilidad de bloqueo.

Esto será esencial más adelante cuando usemos poll().

******Profundización:
    Qué es fcntl
        fcntl significa file control → control de archivos.

        Es una función del sistema POSIX (Unix/Linux/macOS) que sirve para
modificar el comportamiento de un descriptor de archivo (file descriptor, o fd).

        Y recuerda que en Unix todo es un archivo:
            un archivo normal (de disco),

            un socket de red,

            una tubería (pipe),

            incluso el teclado o la pantalla…

    Todos se manejan con descriptores de archivo (int).

*/

/*
Qué tienes hasta ahora

    Has creado un objeto Server capaz de:

    Crear un socket TCP.

    Asociarlo a un puerto.

    Escuchar conexiones sin bloquear.

    Cerrar todo ordenadamente al destruir el objeto.

➡️ Todavía no acepta clientes ni responde datos, pero ya es un servidor
inicializado que escucha. Lo siguiente será crear un main.cpp que lo use y
añadir el bucle principal (aceptar conexiones y enviar un “Hello world”).*/

// VERSION 2 DEL BUCLE RUN
void Server::run() {
  std::cout << "Servidor corriendo con poll()...\n" << std::endl;

  while (g_running) {
    int ready = _pollManager.wait(5000);
    // antes tenia -1, significa que espera indefinidamente
    // hasta que algo ocurra. Pongo 5 segundos de timeout, evita
    // que el servidor se quede bloqueado indefinidamente y
    // permite revisar timeouts periódicamente
    // ready es el número de fds listos para lectura
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      perror("poll");
      break;
    }
    // actualizamos el tiempo actual
    time_t now = time(NULL);

    // El vector de poll tiene una estructura fija al inicio y dinámica después:
    // 1. Los primeros FDs ([0] a [_serverSockets.size() - 1]) son siempre los
    //    sockets servidores que escuchan nuevas conexiones.
    // 2. A partir de ahí, el resto de FDs son dinámicos y pueden ser:
    //    - Sockets de clientes conectados.
    //    - Pipes de salida de procesos CGI en ejecución.

    // _serverSockets.size() es el número de sockets servidores
    // _pollManager.getSize() es el número total de fds

    // FASE 1: Revisar sockets servidores para aceptar nuevas conexiones
    for (size_t i = 0; i < _serverSockets.size(); ++i) {
      short revents = _pollManager.getRevents(i);
      if (revents & POLLIN) {
        acceptNewClient(_pollManager.getFd(i));
      }
    }

    // FASE 2: Procesar clientes existentes - Recorremos el resto revisando
    // todos los clientes Y los pipes CGI
    for (size_t i = _serverSockets.size(); i < _pollManager.getSize();) {
      int fd = _pollManager.getFd(i);

      // ====== CGI PIPE CHECK ======
      // Si este FD es un pipe CGI, manejarlo de forma especial
      std::map<int, ClientConnection *>::iterator cgiIt =
          _cgiPipeToClient.find(fd);
      if (cgiIt != _cgiPipeToClient.end()) {
        ClientConnection *client = cgiIt->second;
        short revents = _pollManager.getRevents(i);
        if (revents & (POLLIN | POLLHUP | POLLERR)) {
          handleCGIPipe(fd, client);
        }
        ++i;
        continue; // No procesar como socket de cliente normal
      }

      // ====== REGULAR CLIENT SOCKET ======
      ClientConnection *client = _clientsByFd[fd];

      // 🧹 Si no hay cliente asociado, limpiamos el fd del poll
      if (!client) {
        _pollManager.removeFd(fd);
        continue;
      }

      // ✅ VERIFICACIÓN DE TIMEOUT PRIMERO (MÁS EFICIENTE)
      checkClientTimeout(client, fd, now);

      // Si el cliente sigue activo, procesar eventos
      if (!client->isClosed()) {
        // Errores de conexión
        short revents = _pollManager.getRevents(i);
        if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
          client->markClosed();
          i++; // No te saltas clientes porque cleanupClosedClients() se ejecuta
               // después del bucle completo
          continue;
        }
        // Lectura de datos
        if (revents & POLLIN) {
          handleClientData(client, i);
        }
        // Escritura de datos
        if (revents & POLLOUT) {
          handleClientWrite(client, i);
          // Si se cerró durante la escritura (flushWrite marcó closed si era
          // error/EOF), pasar al siguiente
          if (client->isClosed()) {
            ++i;
            continue;
          }
        }
      }

      // Solo incrementar si no borramos el elemento
      if (i < _pollManager.getSize() && _pollManager.getFd(i) == fd) {
        // Si el cliente tiene datos pendientes en el buffer (Pipelining),
        // no incrementamos 'i' para volver a procesarlo en la misma vuelta
        // o al menos asegurar que se revise.
        if (client && !client->isClosed() &&
            client->isRequestComplete() == false &&
            client->hasPendingWrite() == false) {
          // Si acabamos de terminar una request y queda data, intentamos
          // procesar la siguiente inmediatamente
          // (O simplemente dejamos que el bucle vuelva a pasar por aquí)
        }
        ++i;
      }
    }

    // Limpiar clientes cerrados
    cleanupClosedClients();
  }
}

/*
17.11.25
Ahora, cuando entramos en handle client, dentro se gestiona que si hay algun
error salga de handle y siga con el bucle. El problema, es que si por ejemplo
hay un cliente con datos disponibles para leer y también tenia datos pendientes
para escribir (o el socket es write ready, que casi siempre lo es aunque no
tengas pendientes) tendrá ambos revents, POLLIN y POLLOUT, por lo que después de
handle client podría entrar en el for de POLLOUT y llamar a flusWrite sobre un
cliente ya cerrado dentro de handle client. No es super grave, porque el propio
fluswrite marcaría closed y saldria al encontrar problemas en el send, pero es
innecesario llegar hasta ahí

Recordatorio rápido: poll() puede devolver varios flags a la vez
    poll() no te da “un único evento”. Un mismo revents puede contener POLLIN,
POLLOUT, POLLHUP, POLLERR, etc. al mismo tiempo. Eso significa que en la misma
iteración puedes tener que: leer datos (POLLIN), escribir datos pendientes
(POLLOUT), y además haber recibido un HUP/ERR asíncrono.

    Esa simultaneidad es el origen de la necesidad de orden y cuidado.

2) Qué pasaba antes (tu código original)
    Orden en cada fd:
        if (POLLIN) -> handleClientEvent(fd)
        if (POLLOUT) -> flushWrite()
        if (POLLERR|POLLHUP|POLLNVAL) -> markClosed()

    Problema real posible:
        poll() devuelve POLLIN | POLLOUT (y quizá también HUP/ERR).

        En handleClientEvent() detectas un error (por ejemplo recv() devolvió 0)
y ejecutas client->markClosed() — marcando ya _closed = true.

        Sin comprobar isClosed(), sigues y llegas a la sección if (POLLOUT) y
llamas a flushWrite() sobre un cliente ya marcado como cerrado.

        flushWrite() intentará send() y probablemente falle (EPIPE, ECONNRESET),
volverá false, marcará _closed (otra vez) y al final cleanupClosedClients()
borrará el cliente.

    Consecuencias:
        Llamadas innecesarias a send() sobre sockets que ya deberías considerar
muertos.

        Logs duplicados y flujos inconsistentes.

        En casos más complejos (keep-alive, borrado inmediato) podría provocar
manejar índices/pollfds inválidos si borras dentro del loop sin cuidado.

3) ¿Por qué comprobar errores antes de lectura/escritura?
    Porque hay errores que ocurren entre tus syscalls: el peer puede cerrar o
resetear la conexión justo después del último send() que hiciste, y antes de la
siguiente llamada. poll() refleja ese estado asíncrono con POLLERR/POLLHUP. Si
procesas I/O sin mirar primero esos flags, puedes: intentar recv() o send()
sobre un fd en mal estado, generar errores evitables, hacer trabajo inútil.

    Mirar los flags de error primero evita todo eso: detectas “esto está roto” y
lo marcas para limpieza sin tocarlo.

    Es decir, errores asíncronos (HUP/ERR) se gestionan antes de tocar el
socket, así no intentas I/O en un fd con problemas.

4) ¿Por qué mover ++i al final (o controlarlo manualmente)?
    En la versión nueva gestionas i manualmente (incrementas en cada rama con
++i cuando proceda) para poder continue y no incrementar en ramas donde ya
hiciste erase. Es una forma segura de iterar cuando en algunas ramas haces
erase() del vector _pollFds. Antes tenías un for (i=1; i<_pollFds.size(); ++i) y
en las ramas llamabas erase() seguido de continue. Eso también funcionaba porque
en el continue evitabas el ++i, y la iteración volvía a comprobar el nuevo
_pollFds[i]. La versión nueva simplemente hace explícito el control del i para
evitar confusiones cuando añades condiciones continue en varios puntos — es más
fácil razonar y menos propenso a errores sutiles.

 */

/*
ACTUALIZACIÓN 2 información:

    for (size_t i = 1; i < _pollFds.size(); ++i)
    {
        int fd = _pollFds[i].fd;
        Client *client = _clientsByFd[fd];
        if (!client)
            continue;

Aunque ya está bien protegido en cleanupClosedClients, no debería haber fds sin
un cliente activo asociado, lo ponemos por doble seguridad. En sistemas de red,
puede haber pequeñas desincronizaciones:

    si un cliente se borra justo después de un poll() pero antes de procesar sus
eventos;

    o si ocurre un error no controlado entre readRequest() y
cleanupClosedClients();

    o si en el futuro agregas threads o funciones que manipulan _clientsByFd
fuera del bucle principal.

Si entras en ese caso, es porque tienes una incoherencia:
hay un fd en _pollFds que ya no tiene su Client en _clientsByFd.
Y si simplemente haces continue, ese fd se quedará en _pollFds para siempre,
ocupando espacio y haciendo que poll() lo siga vigilando inútilmente.

Así que sí ✅ — lo correcto es eliminarlo en ese punto.
*/

/*
ACTUALIZACIÓN información:

Tienes un poll() configurado solo con POLLIN, algo como esto:
    pfds[i].events = POLLIN;

Entonces:
    poll() te avisa solo cuando hay algo que leer (datos entrantes).

    Si haces un send() parcial (no todo el buffer se envía) dentro de
handleClientEvent(), y te queda algo pendiente en _writeBuffer, no volverás a
saber cuándo continuar.

👉 Porque poll() solo te despierta con POLLIN, y no con POLLOUT

Qué pasa si un cliente tiene escritura pendiente

Imagina esto:
    El cliente envía una petición → poll() te despierta con POLLIN.

    En handleClientEvent() lees todo, generas respuesta, llamas a
sendResponse().

    flushWrite() intenta enviar los bytes.
        Si todo sale, genial, fin.

        Pero si send() devuelve EAGAIN → guardas el resto en _writeBuffer.

Ahora tienes datos pendientes…
Pero el socket no se marca solo como POLLOUT.
Así que no volverás a entrar para terminar de enviar hasta que el cliente vuelva
a escribir algo. Y probablemente no lo hará → se queda colgado esperando tu
respuesta completa.

Qué debería pasar (con POLLOUT activado)
    Cuando detectas que una respuesta ha quedado pendiente (hasPendingWrite() ==
true), le dices al poll():

    “Oye, también avísame cuando este socket esté listo para escribir.”

    En código:
        pfds[i].events |= POLLOUT;

    Así, en la siguiente iteración de poll(), el kernel te despertará cuando el
socket tenga espacio libre en su buffer y puedas continuar enviando.

        if (pfds[i].revents & POLLIN)
        handleClientEvent(clients[i]);   // leer y preparar respuesta

        if (pfds[i].revents & POLLOUT)
        handleWriteEvent(clients[i]);    // terminar de enviar lo pendiente

Duda común:
    “¿No entrará dos veces (una por POLLIN y otra por POLLOUT) en la misma
vuelta?”

        Sí, puede ocurrir — y de hecho es lo correcto ✅

        Porque un socket puede estar listo para leer y escribir al mismo tiempo.

        Por ejemplo:
            POLLIN: el cliente envió otra petición.

            POLLOUT: todavía tienes datos pendientes de la respuesta anterior.

        👉 Pero eso no es un problema.
        En esa iteración simplemente procesas ambos eventos:
            lees lo que haya (handleClientEvent) y luego intentas escribir
(flushWrite).

        Lo que debes cuidar es el orden lógico:
            Siempre lee primero (POLLIN) — así vacías el buffer de entrada.

            Luego escribe (POLLOUT) — así respondes cuando haya espacio libre.

Qué pasa cuando terminas de escribir todo
    En cuanto flushWrite() termina y ya no hay nada pendiente:

    if (!client.hasPendingWrite())
        pfds[i].events &= ~POLLOUT; // desactiva interés en escritura

    Así, el poll() ya no seguirá avisándote por POLLOUT,
    hasta que haya una nueva respuesta por enviar.

    Esto mantiene el bucle eficiente y evita que poll() te despierte sin
necesidad.

****DUDA: En el caso de una sola peticion, eso activa pollin, luego envio
respuesta y se queda a medias, para la siguiente vuelta sigue activo pollin de
esa misma petición o se desactiva si no hay mas peticiones y entonces como hay
cosas pendientes se activa solo el pollout y tengo que detectarlo?

Caso: llega una única petición
    Supón este flujo paso a paso:
        Cliente conecta y envía su petición HTTP.
        → El kernel marca el socket con POLLIN porque hay datos listos para
leer.

        Tu poll() despierta (por ese POLLIN).
        → En tu bucle lo detectas y llamas a handleClientEvent().
        → Lees todo con recv(), generas la respuesta y llamas a sendResponse().

        sendResponse() intenta enviar con send().
            Si se envía todo, no pasa nada raro: limpias buffer, fin.

            Si se queda a medias (EAGAIN / EWOULDBLOCK) → guardas el resto en
_writeBuffer.

    Hasta aquí bien, pero ahora pasa lo que tú preguntas 👇

¿Qué pasa con los eventos pollin y pollout después de eso?

🟩 POLLIN

    Una vez que tú lees todo lo que había del socket (con recv() hasta que
devuelve EAGAIN o 0), entonces ya no queda nada en el buffer de lectura.

    Por tanto:
        El kernel deja de marcar POLLIN automáticamente.

        Tu poll() ya no te avisará más por ese socket hasta que el cliente envíe
más datos.

    👉 Es decir: si no hay más peticiones, no volverás a entrar por POLLIN.

🟥 POLLOUT

    Por otro lado, si en el paso anterior tu send() devolvió EAGAIN,
    el kernel te está diciendo básicamente:

        “No puedo escribir ahora, el buffer de salida del socket está lleno.
        Avísame cuando haya espacio libre.”

    Pero ojo: el kernel no activa automáticamente POLLOUT.
    Tienes que decírselo tú, añadiéndolo al events de ese socket:
        pfds[i].events |= POLLOUT;

    Entonces, en la próxima llamada a poll(),
    el kernel te despertará cuando el socket vuelva a estar listo para escribir.

Qué ocurre en la siguiente vuelta del bucle
    Como ya no hay nada que leer (no más POLLIN),
    el único motivo por el que poll() te despertará será:

    ➡️ porque el socket ahora tiene espacio libre para escribir (POLLOUT).

    Entonces tú detectas:
        if (pfds[i].revents & POLLOUT)
            client->flushWrite();

    Y ahí envías lo que te quedaba pendiente en _writeBuffer.
    Cuando terminas (ya se envió todo), haces:

        pfds[i].events &= ~POLLOUT;

    Y el socket vuelve a estar solo con POLLIN activado,
    esperando nuevas peticiones.

Entonces, si se queda a medias, ¿el pollin se desactiva y se activa pollout?
    ✅ Exactamente.
    El kernel deja de marcar POLLIN porque ya leíste todo,
    y tú, manualmente, activas POLLOUT para que te avise cuando puedas seguir
enviando.

Resumen rápido

No, no todo lo hace el kernel automáticamente.
👉 El kernel activa o desactiva dinámicamente los “revents”,
pero no cambia tu configuración “events”.
Tú tienes que decidir qué tipo de eventos quieres monitorizar en cada momento.

🧠 Diferencia entre events y revents

| Campo     | Quién lo maneja | Qué significa | | --------- | --------------- |
------------------------------------------------------------------------------------
| | `events`  | Tú (tu código)  | Qué condiciones quieres que `poll()` vigile
(por ejemplo: `POLLIN`, `POLLOUT`, etc.) | | `revents` | El kernel       | Qué
condiciones **se cumplieron realmente** cuando `poll()` despertó. |

Cuando el socket se queda sin datos (ya leíste todo)
    Después de hacer recv() y vaciar el buffer,
    el kernel simplemente ya no marcará POLLIN en el próximo revents.

    Pero no tienes que quitar POLLIN de events.
    ¿Por qué?
    Porque si luego el cliente te manda otra petición,
    el kernel lo detectará automáticamente y pondrá revents |= POLLIN otra vez.

    👉 Así que mantener POLLIN siempre activo es normal.

Cuando el socket se llena al enviar (EAGAIN)
    Si haces send() y devuelve EAGAIN, significa:
        “No hay espacio ahora en el buffer de salida.”

    Aquí sí tienes que actuar tú:
    añadir POLLOUT a events para que el kernel te avise cuando el socket vuelva
a estar listo. pfds[i].events |= POLLOUT;

    Entonces, cuando el socket tenga espacio libre, en la siguiente vuelta de
poll() el kernel pondrá: pfds[i].revents |= POLLOUT;

Cuando terminas de enviar todo
    En flushWrite(), cuando confirmas que ya no queda nada pendiente
(!hasPendingWrite()):

    Tú misma debes quitar el flag POLLOUT de events:
        pfds[i].events &= ~POLLOUT;

    ¿Por qué?
        Porque si lo dejas activo, el kernel te seguirá “despertando” por
POLLOUT todo el rato, ya que los sockets TCP casi siempre están listos para
escribir. Te haría gastar CPU innecesariamente.

Entonces…
    👉 POLLIN: lo activas una vez y lo dejas siempre.
    El kernel decide si hay algo que leer o no, y pone/quita en revents según
toque. No tienes que cambiarlo tú.

    👉 POLLOUT: lo activas y desactivas manualmente según el estado de tu
_writeBuffer.

*/

/* EXPLICACION

Actualizar la clase Server
Hasta ahora, tu Server:
    Crea el socket.
    Lo asocia a un puerto (bind).
    Empieza a escuchar (listen).
    Acepta conexiones (accept).

Pero solo acepta una conexión y no gestiona múltiples clientes simultáneamente.
Si aceptas un cliente, hasta que no terminas con él no puedes aceptar otro. Si
dos clientes se conectan casi a la vez, el segundo tendrá que esperar hasta que
termines con el primero. Mientras tanto, tu servidor no hace nada más: no puede
recibir otros mensajes ni atender más sockets, porque estás bloqueada en el
flujo “uno a uno”. Aunque tenga el socket como no bloqueante y el accept() no se
queda colgado esperando, porque tienes el continue, aun así tu código no
atenderá a más de un cliente a la vez porque: No guardas los clientFd para
seguir leyendo de ellos. No tienes ninguna lógica que diga: “ahora voy a leer
del cliente 1”, “ahora del cliente 2”. Solo haces accept → send → close. Aunque
no te bloquees esperando conexiones, tampoco gestionas múltiples clientes
simultáneamente. Así que ahora toca hacerlo capaz de manejar varios clientes a
la vez, sin que uno bloquee a los demás.

Para eso lo que haremos ahora es pasar el servidor a usar poll().

pollfd es una estructura definida en <poll.h> que contiene:

struct pollfd
{
    int fd;        // el descriptor de socket
    short events;  // qué eventos queremos vigilar (lectura, escritura...)
    short revents; // qué eventos ocurrieron realmente
};

Qué significa “usar poll()”

    poll() permite vigilar varios file descriptors (FDs) a la vez:

    uno para el socket del servidor (esperando nuevas conexiones),

    y varios para los clientes (esperando datos que leer o que enviar).

Piensa en poll() como un vigilante que está atento a varios sockets a la vez y
te avisa cuando ocurre algo interesante:

    alguien quiere conectarse,

    un cliente ha mandado datos,

    un cliente se ha desconectado…

Entonces tú puedes actuar sin quedarte bloqueada esperando.

Así, en cada ciclo:

    Si el socket del servidor tiene actividad → significa que hay un nuevo
cliente que quiere conectarse→ haces accept() y lo añades a tu lista de pollfd.

    Si un cliente tiene actividad → lees su petición con readRequest().

    Si la petición está completa → generas una respuesta y se la envías con
sendResponse().

👉 el socket (la puerta real de comunicación)
👉 y poll() (el vigilante que observa esas puertas).

Vamos a crear:

    Un std::vector<pollfd> pollFds; → lista de sockets que estamos vigilando.
        En el índice 0 pondremos el socket del servidor (el que hace listen()).
        En los siguientes, los clientes aceptados.

Cada iteración del bucle:

    Llamamos a poll(pollFds.data(), pollFds.size(), -1)
    (espera indefinidamente hasta que haya algo que hacer).
    Recorremos pollFds:
        Si el fd es el del servidor → hay una nueva conexión (accept()).
        Si es otro → ese cliente ha mandado algo o está listo para recibir
respuesta.

**** CÓDIGO: Explicación línea a línea (lo esencial)

1.
Creamos un pollfd para el socket del servidor y registramos POLLIN (nos interesa
cuando haya nuevas conexiones).

Guardamos ese pollfd en _pollFds en la posición 0: convenimos que índice 0 será
siempre socket servidor.

2.
poll(_pollFds.data(), _pollFds.size(), -1)
→ le pasamos todos los fds a vigilar y -1 indica “esperar indefinidamente”.
bloquea hasta que haya eventos en alguno de los fds o hasta que una señal
interrumpa (EINTR).

Si errno == EINTR → reacción adecuada: volver a llamar a poll() (esto evita
terminar por un SIGALRM u otra señal). EINTR significa “Interrupted system call”
→ una llamada al sistema fue interrumpida por una señal antes de completarse
(por ejemplo, accept(), read(), poll(), etc.). Normalmente solo implica volver a
intentarla.

Si hay otro error → imprimimos y salimos del bucle.

ready indica cuántos fds tienen revents no nulos, pero no lo usamos directamente
para optimizar el escaneo.

3. if (_pollFds.size() > 0 && (_pollFds[0].revents & POLLIN))
            acceptNewClient();

Asegura que el vector _pollFds no esté vacío (que haya al menos un socket
registrado).

_pollFds[0] → el primer elemento del vector (tu socket del servidor).

.revents → campo que poll() rellena con los eventos que han ocurrido.
        Cuando haces poll(_pollFds.data(), _pollFds.size(), -1); el kernel
rellena el campo revents de cada pollfd con los eventos que han sucedido (por
ejemplo, si hay datos para leer, una desconexión, un error, etc). Events lo pone
el programador, es lo que quiere vigilar ej. POLLIN para lectura, POLLOUT para
escritura). Revents lo rellena el propio poll(), significa qué ha pasado de
verdad (ej. si llega POLLIN, hay datos listos).

        En el caso del servidor, estás preguntando: “¿Hay algo que pueda leer
ahora en el socket del servidor?” Para un socket de servidor eso no significa
“hay datos de texto o HTML”, sino si hay una nueva conexión pendiente que puedo
aceptar con accept()

POLLIN → bandera que indica “hay datos para leer” (en este caso, una nueva
conexión entrante).

El operador & (AND bit a bit) sirve para comprobar si el bit de POLLIN está
activado.

significa:
👉 “Si hay al menos un socket registrado y el socket del servidor tiene un evento
POLLIN (una nueva conexión entrante), entonces acepto esa conexión.”

***Duda:  pero como se llega a saber que alguien se quiere conectar? como llega
esa información al fd del servidor? Cuando creas un socket de servidor,
_serverFd no es solo un número cualquiera, es un descriptor de archivo que el
kernel asocia con tu aplicación. listen() le dice al kernel: “Todas las nuevas
conexiones que lleguen al puerto X, guárdalas aquí en una cola, y cuando el
programa pregunte, se la damos”. Cuando un cliente hace: connect(server_ip,
port);

    Se inicia el handshake TCP (SYN → SYN-ACK → ACK).
    Una vez completado, el kernel del servidor crea una entrada en la cola de
conexiones pendientes. _serverFd sigue siendo el mismo descriptor, pero ahora el
kernel sabe que hay algo “listo para leer” en ese descriptor: una conexión que
se puede aceptar.

💡 Por eso, en poll(), _pollFds[0].revents & POLLIN se activa:
    “el socket tiene algo que ‘leer’ → hay una conexión esperando que aceptes”

Al llamar accept():
    Saca la primera conexión de la cola.
    Crea un nuevo socket (clientFd) dedicado a ese cliente.
    _serverFd sigue existiendo y puede aceptar más conexiones nuevas.

Es decir:
    _serverFd → puerta de entrada general (escucha nuevas conexiones)
    clientFd → puerta de entrada personalizada para ese cliente concreto

***Fin duda****

Si hay un nuevo cliente esperando a ser aceptado, entramos en acceptNewClient()

4.
for (size_t i = 1; i < _pollFds.size(); ++i)
{
    if (_pollFds[i].revents & POLLIN)
        handleClientEvent(_pollFds[i].fd);

Despues se recorren el resto de índices de la lista _pollFds, que son los
clientes ya conectados, para evaluar si hay algun revent tipo Pollin (peticiones
que haya pendientes de leer). En tal caso, se llama a handleClientEvent

5. Revisar si en el proceso ha habido clientes que se han cerrado y hay que
limpiar
}

*/

void Server::checkClientTimeout(ClientConnection *client, int fd, time_t now) {
  const int CLIENT_TIMEOUT = 30;
  if (client->isTimedOut(now, CLIENT_TIMEOUT)) {
    std::cout << "[Timeout] Cliente fd " << fd << " inactivo más de "
              << CLIENT_TIMEOUT << " segundos, cerrando.\n";
    client->markClosed();
  }
}

void Server::acceptNewClient(int serverFd) {
  while (true) {
    sockaddr_in clientAddr; // almacena IP y puerto del cliente que se conecta.
    socklen_t clientLen =
        sizeof(clientAddr); // tamaño de esa estructura, necesario para accept()
    int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break; // ya no hay más conexiones pendientes
      perror("accept");
      break;
    }

    // Poner el socket del cliente en modo no bloqueante
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
      std::cerr << "Error poniendo socket de cliente en modo no bloqueante"
                << std::endl;
      close(clientFd);
      continue;
    }

    // Pass the configs associated with this serverFd to the new Client
    ClientConnection *client = new ClientConnection(
        clientFd, clientAddr, _configsByServerFd[serverFd]);
    _clientsByFd[clientFd] = client;

    _pollManager.addFd(clientFd, POLLIN);

    std::cout << "Nueva conexión (fd: " << clientFd
              << ", IP: " << client->getIp() << ")" << std::endl;
  }
}

/*
¿Por qué un bucle accept()?
    poll() te dice "hay conexiones pendientes", pero puede haber más de una
esperando, por eso es un bucle infinito. En non-blocking debes accept() en bucle
hasta que accept() devuelva -1 con EAGAIN (ya no hay más conexiones pendientes).
Si no haces el bucle, te quedarías con conexiones sin aceptar hasta la próxima
llamada a poll().

Pasos por nuevo cliente:
    accept() → crea un nuevo socket (fd nuevo) para hablar con ese cliente. Lo
que hace accept() internamente es tomar la conexión pendiente de la cola del
kernel y rellenar clientAddr con la dirección del cliente que se conectó
(dirección IPv4, la IP del cliente y el puerto del cliente) y ajustar el
clientLen al tamaño real de los datos escritos en clientAddr _serverFd →
descriptor del socket del servidor, escuchando en algún puerto (ej. 8080).
        (struct sockaddr*)&clientAddr → cast porque accept espera un puntero a
sockaddr genérico. clientLen → indica el tamaño de la estructura de dirección.

        Resultado:
            Si hay una conexión pendiente → devuelve un nuevo fd (clientFd) para
hablar con ese cliente. Si no hay → devuelve -1 y se setea errno. Si clientFd ==
-1 y errno es EAGAIN/EWOULDBLOCK → significa "no hay más conexiones ahora" →
rompemos el accept-loop. Puede dar esto gracias a que está en modo no
bloqueante. Por el contrario, perror("accept") → cualquier otro error real lo
imprime en consola.

    Convertimos clientFd a non-blocking.
        Esto permite que no se bloquee cuando intentemos leer o escribir datos
en ese socket más adelante. Fundamental para poder atender muchos clientes a la
vez con un solo hilo.

    Creamos Client* c = new Client(clientFd); y lo guardamos en
_clientsByFd[clientFd]. Client → clase que encapsula información del cliente
(fd, IP, buffer de lectura, etc.). Se guarda en _clientsByFd con clave clientFd.
        Así puedes acceder rápidamente al cliente según su descriptor de socket.
        💡 Nota: se usa puntero (Client*) para no copiar la clase y poder
manejarla dinámicamente.

    struct pollfd pfd = {clientFd, POLLIN, 0};
    _pollFds.push_back(pfd);
        pollfd → estructura que poll() necesita.
        fd → el descriptor del cliente.
        events → eventos que queremos vigilar, aquí POLLIN (datos listos para
leer). revents → inicializado a 0, lo rellena poll() luego. Añadimos el nuevo
socket (fd) a la lista _pollFds para que poll() empiece a vigilar este cliente
también.

***DUDA: _clientsByFd y _pollFds sirven para cosas distintas y complementarias.
    Cuando aceptas un cliente
        accept() te da un clientFd
        Lo guardas en _pollFds para que poll() lo vigile
        Lo guardas también en _clientsByFd para poder acceder a su objeto
después

Cuando poll() te dice “hay algo en fd = 7”, solo sabes que ahí hay datos, pero ú
necesitas el objeto cliente que representa ese fd, para llamar a sus funciones.
Por eso en handleClientEvent(fd) haces Client* client = _clientsByFd[fd]; Y
ahora puedes: leer del socket (client->readRequest()) enviar respuesta
(client->sendResponse()) actualizar _lastActivity etc.

Cuando detectas que un cliente cerró su conexión o que hay error, tienes que
eliminarlo de ambos sitios

Así liberas memoria y evitas que poll() siga vigilando un socket muerto.

*/

void Server::handleClientData(ClientConnection *client, size_t pollIndex) {
  // 1. Leer datos del socket
  if (!client->readRequest())
    return; // error o desconexión del cliente → cleanup lo limpiará->
            // marcó closed

  // 2. Si la petición está completa, procesar la request y generar la respuesta
  // + enviar la respuesta
  if (client->isRequestComplete()) {
    if (!client->processRequest() || !client->sendResponse())
      return; // Error -> ya marcó closed, cleanup lo limpiará después

    // === CGI ASYNC REGISTRATION ===
    if (client->getCGIState() == CGI_RUNNING) {
      int pipeFd = client->getCGIPipeFd();
      if (pipeFd != -1 &&
          _cgiPipeToClient.find(pipeFd) == _cgiPipeToClient.end()) {
        _pollManager.addFd(pipeFd, POLLIN);
        _cgiPipeToClient[pipeFd] = client;
      }
    }
  }

  // 3. Si queda algo por enviar, activar POLLOUT para que handleClientWrite
  // termine el trabajo
  if (client->hasPendingWrite()) {
    _pollManager.updateEvents(pollIndex, POLLIN | POLLOUT);
  }
}

void Server::handleClientWrite(ClientConnection *client, size_t pollIndex) {
  // Intentar vaciar el buffer de salida
  if (!client->flushWrite())
    return; // Error -> se marcó closed y cleanup lo limpiará

  // Actualizar eventos -> Si ya no queda nada pendiente por enviar, desactivar
  // POLLOUT
  if (!client->hasPendingWrite()) {
    _pollManager.updateEvents(pollIndex, POLLIN);
  }
}
/*
Solo podemos detectar si el cliente se ha cerrado por su lado en el momento de
intentar leer (recv()) o de intentar escribir (send()), y eso solo pasa en
readrequest y en fluswrite, por lo tanto lo checkeamos despues de ambas
funciones, pero entremedias no tiene sentido hacerlo, solo si lo hemos cerrado
nosotros expresamente por un error. El error no lo podre saber hasta que envie o
reciba algo, es normal, todos los servidores funcionan así. Nunca se arrastra
sin detectarlo antes de usar el socket
*/

/*
14.11.25
Actualización de responsabilidades que tendrá que hacer client:
    1. readRequest()
        Recibe bytes y los pasa al parser (HttpRequest).

    2. processRequest()
        Cuando HttpRequest dice que está completa → decides qué respuesta toca.
        Aquí se crea/llena HttpResponse.

    3. sendResponse()
        Convierte el HttpResponse en string, lo envía y resetea para siguiente
petición.
*/

/*
Para que sea mas sencillo, asignamos un puntero client que señala al objeto
Client correspondiente al fd que llega como argumento. Si el cliente con ese fd
existe, se guarda su puntero en client*

if (!client->readRequest()) return;
Esta línea es clave, se llama a readRequest para:
    Se lee del socket del cliente (recv) todo lo que ha llegado hasta ahora.
    Se acumula en un buffer interno (_requestBuffer).
    Si todavía no ha llegado todo el mensaje (por ejemplo, si el cliente no ha
enviado aún todo el encabezado HTTP), devolvemos false y esperamos a la próxima
vez que poll() diga que hay más datos. Cuando la petición está completa (por
ejemplo, ya se recibió el doble salto de línea \r\n\r\n que marca el final de
los headers HTTP), devuelve true.

    👉 Si devuelve false, el servidor no responde todavía, sale y solo espera más
datos la próxima vez.

***DUDA: PERO SI AUN FALTA POR LLEGAR, NO TENEMOS QUE ENTRAR MAS A READREQUEST,
POR SI VENIA POR PARTES O NO HA PODIDO LEERLO TODO PORQUE EL BUFFER ERA MAS
PEQUEÑO QUE EL TAMAÑO DE LA PETICION? Tu intuición es totalmente correcta: el
servidor no se queda bloqueado esperando a que llegue el resto, sino que vuelve
al bucle principal. Pero eso no significa que la petición se “olvide”: el
cliente sigue registrado y poll() lo volverá a despertar cuando haya más datos
disponibles.

    if (!client->readRequest())
        return; // aún no ha llegado todo
    … significa:
        “Aún no tengo toda la petición, así que no hago nada más por ahora”.

    Luego el flujo continúa:
        Sales de handleClientEvent().
        El bucle principal (poll()) sigue iterando y escuchando todos los
descriptores (server y clientes). En la siguiente vuelta, cuando el cliente
mande más datos, poll() marcará su socket con POLLIN. Entonces
handleClientEvent(fd) se volverá a llamar automáticamente para ese cliente, y
esta vez readRequest() añadirá el nuevo trozo a _request.

    Así, poco a poco se va completando la petición.

    👉 Esto es no bloqueante y reactivo: nunca te quedas “esperando dentro” de
una función.
***FIN DUDA

*/

void Server::cleanupClosedClients() {
  // Recorrer todos los clientes
  for (std::map<int, ClientConnection *>::iterator it = _clientsByFd.begin();
       it != _clientsByFd.end();) {
    int fd = it->first;                    // Obtener el descriptor de archivo
    ClientConnection *client = it->second; // Obtener el puntero al cliente

    if (client->isClosed()) {
      std::cout << "Cerrando conexión fd: " << fd << std::endl;

      // Cleanup any associated CGI pipe
      int pipeFd = client->getCGIPipeFd();
      if (pipeFd != -1) {
        std::cout << "[Server] Cleaning up CGI pipe " << pipeFd
                  << " for client fd " << fd << std::endl;
        _pollManager.removeFd(pipeFd);
        _cgiPipeToClient.erase(pipeFd);
      }

      _pollManager.removeFd(fd); // Eliminar el descriptor de archivo
      delete client;             // Liberar la memoria del cliente
      _clientsByFd.erase(it++);  // Eliminar el cliente de la lista
    } else {
      ++it;
    }
  }
}

/**
 * @brief Handles CGI pipe data when poll() detects POLLIN on a CGI pipe
 *
 * This is called when there's data to read from a running CGI process.
 * Reads available data, handles EOF (CGI done), and triggers response building.
 */
void Server::handleCGIPipe(int pipeFd, ClientConnection *client) {
  if (!client || client->getCGIState() != CGI_RUNNING) {
    return;
  }

  // Read available data from CGI pipe (non-blocking)
  bool readOk = client->readCGIOutput();

  // Check if CGI is done (EOF reached)
  if (client->getCGIState() == CGI_DONE) {
    // Reap zombie process with WNOHANG
    pid_t pid = client->getCGIPid();
    if (pid > 0) {
      int status;
      waitpid(pid, &status, WNOHANG);
    }

    // Remove CGI pipe from poll and tracking map
    _pollManager.removeFd(pipeFd);
    _cgiPipeToClient.erase(pipeFd);

    // Build HTTP response from CGI output
    CGIHandler cgiHandler;
    HttpResponse response =
        cgiHandler.buildResponseFromCGIOutput(client->getCGIBuffer());

    // Set the response in the client's write buffer
    std::string responseStr = response.buildResponse();
    client->setCGIResponse(responseStr);

    // Activate POLLOUT to send response
    int clientFd = client->getFd();
    _pollManager.updateEvents(clientFd, POLLIN | POLLOUT);
  }
  (void)readOk; // Suppress unused warning for now
}
