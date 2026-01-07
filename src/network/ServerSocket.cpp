#include "network/ServerSocket.hpp"

ServerSocket::ServerSocket(int port) : _fd(-1), _port(port) {}

ServerSocket::~ServerSocket() { closeSocket(); }

bool ServerSocket::init() {
  // 1. Crear socket (IPv4, TCP)
  // AF_INET: Familia de direcciones IPv4.
  // SOCK_STREAM: Tipo de socket orientado a conexión (TCP).
  // 0: Protocolo por defecto (TCP).
  _fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_fd < 0) {
    std::cerr << "❌ Error creando socket para puerto " << _port << ": "
              << strerror(errno) << std::endl;
    return false;
  }

  // 2. Configurar socket para reutilizar puerto (SO_REUSEADDR y SO_REUSEPORT)
  // SO_REUSEADDR: Permite reiniciar el servidor sin esperar a que el puerto se
  // libere del estado TIME_WAIT. Sin esto, si paras y arrancas rápido, el SO
  // podría decir "Address already in use".
  // SO_REUSEPORT: (Especialmente útil en
  // macOS/BSD) permite que varios sockets se vinculen exactamente al mismo
  // puerto.
  int opt = 1;
  if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "❌ Error configurando SO_REUSEADDR en puerto " << _port << ": "
              << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }
  // REVISAR SI ES NECESARIO EN UBUNTU!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef SO_REUSEPORT
  if (setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    std::cerr << "❌ Error configurando SO_REUSEPORT en puerto " << _port << ": "
              << strerror(errno) << std::endl;
    // No salimos aquí porque SO_REUSEPORT no es crítico si SO_REUSEADDR
    // funcionó
  }
#endif

  // 3. Poner en modo no bloqueante
  // Evita que funciones como accept(), recv() o send() detengan la ejecución
  // del programa. Es esencial para poder manejar múltiples clientes
  // simultáneamente con poll().
  if (setNonBlocking(_fd) < 0) {
    std::cerr << "❌ Error poniendo socket en modo no bloqueante (puerto "
              << _port << "): " << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }

  // 4. Definir dirección y puerto
  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(_port);

  // 5. Bind: Asocia el socket a la dirección y puerto configurados.
  // Si falla, suele ser porque el puerto ya está en uso por otro programa.
  if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    std::cerr << "❌ Error en bind en puerto " << _port << ": "
              << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }

  // 6. Listen: Pone el socket en modo escucha para aceptar conexiones
  // entrantes. SOMAXCONN define el tamaño máximo de la cola de conexiones
  // pendientes.
  if (listen(_fd, SOMAXCONN) < 0) {
    std::cerr << "❌ Error en listen en puerto " << _port << ": "
              << strerror(errno) << std::endl;
    closeSocket();
    return false;
  }

  return true;
}

int ServerSocket::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int ServerSocket::getFd() const { return _fd; }

int ServerSocket::getPort() const { return _port; }

void ServerSocket::closeSocket() {
  if (_fd != -1) {
    close(_fd);
    _fd = -1;
  }
}
