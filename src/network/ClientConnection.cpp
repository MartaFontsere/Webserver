#include "network/ClientConnection.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
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
      _requestComplete(false), _servCandidateConfigs(servCandidateConfigs) {
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
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Si devuelve esto, no significa error, no hay datos ahora mismo (socket
      // non-blocking). Por eso no tenemos que cerrar el socket en este caso.
      return true;
    }
    std::cerr << "[Error] Fallo al leer del cliente con recv() (fd: "
              << _clientFd << "): " << strerror(errno) << "\n";
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
    // ✅ Verificar SI el parse fue exitoso PERO con error de tamaño (413)
    if (_httpRequest.isBodyTooLarge()) {
      _httpResponse.setErrorResponse(413);
      // applyConnectionHeader(); // TODO: Revisar si ClientConnection debe
      // manejar esto o RequestHandler
      _requestComplete = true; // Marcar como completa PARA RESPONDER el error
      return true;             // No es error fatal, hay respuesta que enviar
    }
    std::cout << "✅ Request completa (client fd: " << _clientFd << ")\n";
    _requestComplete = true;
    _rawRequest.clear(); // Limpiamos el buffer raw para la próxima request
  }
  // TODO: IMPORTANTEEEEEEE!!!!
  // Nota: más adelante, si quieres soportar pipelining, cambia esto por
  // _rawRequest.erase(0, parsedBytes) y haz que HttpRequest devuelva
  // parsedBytes.
  return true;
}

bool ClientConnection::processRequest() {
  if (!_requestComplete)
    return true;

  _httpResponse =
      _requestHandler.handleRequest(_httpRequest, _servCandidateConfigs);

  // Prepare write buffer
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
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // El kernel nos dice: "Vuelve más tarde, ahora mismo no puedo aceptar más
      // bytes". No es un error, simplemente el buffer de salida está lleno.
      return true;
    }
    // Error serio: marca cerrado para cleanup
    std::cerr << "[Error] send() fallo (fd: " << _clientFd
              << "): " << strerror(errno) << "\n";
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
  _rawRequest.clear();
  _writeBuffer.clear();
  _writeOffset = 0;
}
