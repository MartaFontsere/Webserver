#include "network/ClientConnection.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno> // para errno (Variable global que contiene el código del último error de una llamada al sistema), EAGAIN (Constante que indica "Resource temporarily unavailable" (común en operaciones no bloqueantes)), EWOULDBLOCK (Constante que indica "Operation would block" (común en operaciones no bloqueantes))
#include <csignal>
#include <cstring> // para strerror (Función que devuelve una cadena descriptiva del último error de una llamada al sistema)
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

/*
¿Por qué necesitamos ClientConnection.cpp?

Cuando tu servidor recibe una conexión (accept()), obtiene un nuevo file
descriptor (FD) que representa a ese cliente específico. Pero el servidor puede
tener muchos clientes conectados al mismo tiempo. → Por tanto, necesitamos una
forma clara de guardar y gestionar la información de cada cliente: su FD, su
estado (si está leyendo o escribiendo), lo que ha enviado, lo que hay que
responderle, etc.

La clase ClientConnection sirve justo para eso: encapsula todo lo que pasa con
un cliente concreto dentro de un objeto. Así evitamos caos y código duplicado
dentro del servidor.
*/

/*
Objetivo: que ClientConnection solo lea bytes del socket y no se encargue de
entender HTTP. La lógica de parseHeaders, Content-Length, etc. se delega a la
clase HttpRequest.
*/

ClientConnection::ClientConnection(
    int fd, const sockaddr_in &addr,
    const std::vector<ServerConfig> &servCandidateConfigs)
    : _clientFd(fd), _addr(addr), _closed(false), _rawRequest(""),
      _writeBuffer(""), _writeOffset(0), _lastActivity(time(NULL)),
      _requestComplete(false), _servCandidateConfigs(servCandidateConfigs),
      _cgiState(CGI_NONE), _cgiPipeFd(-1), _cgiPid(0) {
  /*
  ¿Por qué ahora recibe configs?
    Porque recuerda el flujo real:
      socket (puerto) -> accept() -> ClientConnection(fd, addr,
  configs_de_ese_socket)

    📌 El Server ya ha decidido:
        “este socket corresponde a estos ServerConfig”

    👉 El ClientConnection solo recibe candidatos válidos. No es toda la config,
    es solo la config posible para este puerto.
  */
}

ClientConnection::~ClientConnection() {
  // Cleanup CGI process if running
  if (_cgiPid > 0) {
    std::cout << "[ClientConnection] Killing CGI process " << _cgiPid
              << " for client fd " << _clientFd << std::endl;
    kill(_cgiPid, SIGKILL);
    int status;
    waitpid(_cgiPid, &status, 0);
  }

  // Close CGI pipe if open
  if (_cgiPipeFd != -1) {
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
  }

  if (!_closed) {
    std::cout << "[ClientConnection] Cerrando conexión con " << getIp()
              << std::endl;
    if (_clientFd != -1)
      close(_clientFd);
    _closed = true;
    _clientFd = -1;
  }
}

int ClientConnection::getFd() const { return _clientFd; }

std::string ClientConnection::getIp() const {
  char ipStr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &_addr.sin_addr, ipStr, sizeof(ipStr)) != NULL)
    return std::string(ipStr);
  return "Unknown IP";
}

bool ClientConnection::readRequest() {
  char buffer[4096];
  int bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);

  if (bytesRead < 0) {
    // Si poll() indicó que hay datos con POLLIN y recv() falla, es un error
    // real (No comprobamos errno por requisito del subject)
    std::cerr << "[Error] Fallo al leer del cliente con recv() (fd: "
              << _clientFd << ")\n";
    _closed = true;
    return false;
  } else if (bytesRead == 0) {
    // cliente cerró la conexión por su lado
    std::cout << "[Info] Cliente (fd: " << _clientFd
              << ") cerró la conexión normalmente.\n";
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
  std::cout << "[Debug] Parseando request del cliente fd " << _clientFd
            << std::endl;
  if (_httpRequest.parse(_rawRequest)) {
    // Nota: El límite de body size ahora se verifica en RequestHandler
    // usando location.getMaxBodySize() del config
    std::cout << "✅ Request completa (client fd: " << _clientFd << ")\n";
    _requestComplete = true;

    // Soporte para Pipelining: eliminamos solo la parte procesada
    int parsedBytes = _httpRequest.getParsedBytes();
    if (parsedBytes > 0 && (size_t)parsedBytes <= _rawRequest.size()) {
      _rawRequest.erase(0, parsedBytes);
      std::cout << "[Debug] Pipelining: erased " << parsedBytes
                << " bytes. Remaining in buffer: " << _rawRequest.size()
                << std::endl;
    } else {
      _rawRequest.clear();
    }
  } else if (_httpRequest.headersComplete()) {
    // EARLY BODY SIZE CHECK: Si los headers están listos, ya podemos saber
    // el Content-Length. Si es mayor que el máximo permitido, no seguimos
    // leyendo el cuerpo.
    // Nota: Usamos un valor por defecto conservador (1MB) si no tenemos
    // acceso fácil a la config específica aquí, o dejamos que RequestHandler
    // lo gestione, pero marcamos como completo para que processRequest actúe.
    if (_httpRequest.getContentLength() > 0) {
      // Buscamos el límite en la primera config candidata (como fallback)
      size_t maxBody = 1024 * 1024; // 1MB default
      if (!_servCandidateConfigs.empty()) {
        // Esto es aproximado, RequestHandler hará el check exacto por location
        maxBody = _servCandidateConfigs[0].getClientMaxBodySize();
      }

      if ((size_t)_httpRequest.getContentLength() > maxBody) {
        std::cout << "⚠️ Body too large (" << _httpRequest.getContentLength()
                  << " > " << maxBody << "). Stopping read.\n";
        _requestComplete =
            true; // Forzamos fin para que processRequest de el 413
      }
    }
  }
  return true;
}

bool ClientConnection::processRequest() {
  if (!_requestComplete)
    return true;

  // Guard: If CGI is already running or done (waiting to be sent), don't
  // process again
  if (_cgiState != CGI_NONE)
    return true;

  // Pass 'this' to handleRequest to enable async CGI path
  _httpResponse =
      _requestHandler.handleRequest(_httpRequest, _servCandidateConfigs, this);

  // If CGI is pending, we don't prepare the write buffer yet.
  // The response will be built and set via setCGIResponse() when the CGI
  // finishes.
  if (_httpResponse.isCGIPending()) {
    std::cout << "[ClientConnection] CGI is pending for fd: " << _clientFd
              << std::endl;
    return true;
  }

  // Prepare write buffer for normal (non-CGI or sync CGI) responses
  _writeBuffer = _httpResponse.buildResponse();
  _writeOffset = 0;

  /*
  Poner la generación del string de respuesta en processRequest. Porque así nos
  aseguramos de que se genera una sola vez. Si estuviera en sendResponse, y
  flushWrite no pudiera enviar todo de golpe (EAGAIN), al volver a llamar a
  sendResponse estaríamos regenerando y añadiendo la misma respuesta al buffer
  una y otra vez.
  */

  return true;
}

bool ClientConnection::sendResponse() { return flushWrite(); }
/*
Aquí imprimimos lo que se ha decidido en process request

Explicación del flujo:
1. El servidor llama a sendResponse() cuando ya tienes la respuesta generada.
2. Llama a flushWrite() para intentar enviarla inmediatamente.
3. Si quedan bytes pendientes, el servidor deberá activar POLLOUT para seguir
enviando.
*/

bool ClientConnection::flushWrite() {
  if (_writeBuffer.empty())
    return true;

  // Si ya hemos enviado todo el buffer (doble check de seguridad)
  if (_writeOffset >= _writeBuffer.size()) {
    _writeBuffer.clear();
    _writeOffset = 0;
    std::cout << "[Info] Respuesta completa enviada al cliente (fd: "
              << _clientFd << ")\n";
    return true;
  }

  const char *buf = _writeBuffer.data() + _writeOffset;
  size_t remaining = _writeBuffer.size() - _writeOffset;

  /*
  send() es la llamada al sistema para enviar datos por un socket.
  Como el socket es NO BLOQUEANTE (EAGAIN), puede que no envíe todo de golpe.
  */
  ssize_t s = send(_clientFd, buf, remaining, 0);

  if (s > 0) {
    _writeOffset += static_cast<size_t>(s);
    _lastActivity = time(NULL);

    // Imprimimos lo que estamos enviando (útil para depuración)
    std::cout << "\n[Info] Enviando respuesta al cliente (fd: " << _clientFd
              << "):\n"
              << buf << "\n\n";

    // Si ya hemos enviado todo el buffer tras este send()
    if (_writeOffset >= _writeBuffer.size()) {
      _writeBuffer.clear();
      _writeOffset = 0;
      std::cout << "[Info] Respuesta completa enviada al cliente (fd: "
                << _clientFd << ")\n";

      // 4. Si no queda nada pendiente, todo enviado -> según keep-alive, marcar
      // cerrado o dejar abierto
      if (!_httpRequest.isKeepAlive()) {
        // cerrar la conexión limpiamente (marcar para que cleanup la borre)
        _closed = true;
        std::cout
            << "[ClientConnection] Respuesta completa. Cierre por Connection: "
               "close (fd: "
            << _clientFd << ")" << std::endl;
      } else {
        // mantener la conexión abierta para próximas peticiones
        // además limpiar buffers de request para la siguiente
        resetForNextRequest();
        std::cout
            << "[ClientConnection] Respuesta completa, manteniendo conexión "
               "(keep-alive fd: "
            << _clientFd << ")\n    Esperando nueva request" << std::endl;
      }
    }
    return true;
  } else if (s == -1) {
    // Si poll() indicó que podemos escribir con POLLOUT y send() falla, es un
    // error real (No comprobamos errno por requisito del subject)
    std::cerr << "[Error] Fallo al enviar con send() (fd: " << _clientFd
              << ")\n";
    _closed = true;
    return false;
  } else { // s == 0
    // peer cerró la conexión inesperadamente
    std::cout << "[Info] Cliente (fd: " << _clientFd
              << ") cerró la conexión durante el envío.\n";
    _closed = true;
    return false;
  }
}

bool ClientConnection::hasPendingWrite() const { return !_writeBuffer.empty(); }

void ClientConnection::markClosed() { _closed = true; }

bool ClientConnection::isClosed() const { return _closed; }

bool ClientConnection::isRequestComplete() const { return _requestComplete; }

const HttpRequest &ClientConnection::getHttpRequest() const {
  return _httpRequest;
}

time_t ClientConnection::getLastActivity() const { return _lastActivity; }

bool ClientConnection::isTimedOut(time_t now, int timeoutSec) const {
  return (now - _lastActivity) > timeoutSec;
}

void ClientConnection::resetForNextRequest() {
  /*
  Limpiamos el estado interno para permitir que el cliente envíe otra petición
  por el mismo socket (HTTP/1.1 Keep-Alive).
  */
  _httpRequest.reset();
  _requestComplete = false;
  // _rawRequest.clear(); // COMENTADO: No limpiar para soportar Pipelining
  std::cout << "[Debug] resetForNextRequest: rawRequest size remaining: "
            << _rawRequest.size() << std::endl;
  _writeBuffer.clear();
  _writeOffset = 0;
  // Reset CGI state
  _cgiState = CGI_NONE;
  if (_cgiPipeFd != -1) {
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
  }
  _cgiPid = 0;
  _cgiBuffer.clear();
}

bool ClientConnection::checkForNextRequest() {
  if (_rawRequest.empty())
    return false;

  std::cout << "[Debug] Checking for next request in buffer (size: "
            << _rawRequest.size() << ") for fd " << _clientFd << std::endl;

  // Reset HttpRequest state before parsing to avoid stale state issues
  _httpRequest.reset();

  if (_httpRequest.parse(_rawRequest)) {
    std::cout << "✅ Next request complete in buffer (client fd: " << _clientFd
              << ")\n";
    _requestComplete = true;

    // Soporte para Pipelining: eliminamos solo la parte procesada
    int parsedBytes = _httpRequest.getParsedBytes();
    if (parsedBytes > 0 && (size_t)parsedBytes <= _rawRequest.size()) {
      _rawRequest.erase(0, parsedBytes);
      std::cout << "[Debug] Pipelining (buffer): erased " << parsedBytes
                << " bytes. Remaining: " << _rawRequest.size() << std::endl;
    } else {
      _rawRequest.clear();
    }
    return true;
  }
  return false;
}

// ====== CGI Non-blocking Methods ======

CGIState ClientConnection::getCGIState() const { return _cgiState; }

int ClientConnection::getCGIPipeFd() const { return _cgiPipeFd; }

pid_t ClientConnection::getCGIPid() const { return _cgiPid; }

const std::string &ClientConnection::getCGIBuffer() const { return _cgiBuffer; }

void ClientConnection::startCGI(int pipeFd, pid_t pid) {
  _cgiState = CGI_RUNNING;
  _cgiPipeFd = pipeFd;
  _cgiPid = pid;
  _cgiBuffer.clear();
  std::cout << "[CGI] Started async CGI (pid: " << pid << ", pipe: " << pipeFd
            << ")\n";
}

bool ClientConnection::readCGIOutput() {
  if (_cgiState != CGI_RUNNING || _cgiPipeFd == -1) {
    return false;
  }

  char buffer[4096];
  ssize_t bytesRead = read(_cgiPipeFd, buffer, sizeof(buffer));

  if (bytesRead > 0) {
    _cgiBuffer.append(buffer, bytesRead);
    _lastActivity = time(NULL);
    return true;
  } else if (bytesRead == 0) {
    // EOF - CGI cerró stdout
    std::cout << "[CGI] EOF reached, output size: " << _cgiBuffer.size()
              << " bytes\n";
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
    _cgiState = CGI_DONE;
    return true;
  } else {
    // bytesRead < 0
    // En non-blocking, EAGAIN/EWOULDBLOCK significa "no hay datos aún"
    // Pero el subject prohibe verificar errno - asumimos que si poll()
    // indicó POLLIN y read() falla, es un error real
    std::cerr << "[CGI] Read error on pipe\n";
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
    _cgiState = CGI_DONE;
    return false;
  }
}

void ClientConnection::finishCGI(int exitStatus) {
  (void)exitStatus; // Puede usarse para logging
  _cgiState = CGI_DONE;
  if (_cgiPipeFd != -1) {
    close(_cgiPipeFd);
    _cgiPipeFd = -1;
  }
}

void ClientConnection::setCGIResponse(const std::string &responseStr) {
  // Set the write buffer with the CGI response
  // This allows Server to send the response via normal POLLOUT flow
  _writeBuffer = responseStr;
  _writeOffset = 0;
}
