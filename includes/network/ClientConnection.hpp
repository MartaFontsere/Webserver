#pragma once

#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/RequestHandler.hpp"
#include <ctime>
#include <netinet/in.h>
#include <string>
#include <vector>

/**
 * @class ClientConnection
 * @brief Representa una conexión individual con un cliente.
 *
 * ¿Por qué la necesitamos?
 * Cuando tu servidor recibe una conexión (accept()), obtiene un nuevo file
 * descriptor (FD) que representa a ese cliente específico. Pero el servidor
 * puede tener muchos clientes conectados al mismo tiempo. → Por tanto,
 * necesitamos una forma clara de guardar y gestionar la información de cada
 * cliente: su FD, su estado (si está leyendo o escribiendo), lo que ha enviado,
 * lo que hay que responderle, etc.
 *
 * La clase ClientConnection sirve justo para eso: encapsula todo lo que pasa
 * con un cliente concreto dentro de un objeto. Así evitamos caos y código
 * duplicado dentro del servidor.
 *
 * Responsabilidades (Fase 2 - Refactorizada):
 * 1. Buffering: Acumular datos recibidos del socket (_rawRequest).
 * 2. Parsing: Delegar el parseo progresivo a HttpRequest.
 * 3. Respuesta: Utilizar RequestHandler para generar la respuesta una vez
 *    que la petición está completa.
 * 4. Envío: Gestionar el envío asíncrono de la respuesta al socket
 * (flushWrite).
 */
class ClientConnection {
public:
  ClientConnection(int fd, const sockaddr_in &addr,
                   const std::vector<ServerConfig> &serverCandidateConfigs);
  ~ClientConnection();

  int getFd() const;
  std::string getIp() const;

  bool readRequest();    // lee datos del cliente
  bool processRequest(); // crea HttpResponse basado en HttpRequest
  bool sendResponse();   // envía respuesta
  bool isClosed() const;

  // nuevo: encolar respuesta y vaciar buffer progresivamente
  bool flushWrite();            // intenta enviar bytes pendientes (usa send())
  bool hasPendingWrite() const; // true si queda data por enviar
  void markClosed();
  bool isRequestComplete() const;
  const HttpRequest &getHttpRequest() const;

  // timeout helpers
  time_t getLastActivity() const;
  bool isTimedOut(time_t now, int timeoutSec) const;

  // preparar para la próxima request cuando hay keep-alive
  void resetForNextRequest();

private:
  int _clientFd;     // file descriptor del socket del cliente
  sockaddr_in _addr; // dirección IP y puerto del cliente
  bool _closed;      // indica si la conexión está cerrada

  // lectura/parseo
  std::string _rawRequest; // buffer con los datos RAW recibidos del socket
  HttpRequest _httpRequest;

  //  salida (write buffering)
  std::string _writeBuffer; // Los datos pendientes de enviar
  size_t _writeOffset;  // bytes ya enviados desde el inicio de _writeBuffer.
  time_t _lastActivity; // timestamp del último recv/send exitoso.
  bool _requestComplete;
  std::vector<ServerConfig> _servCandidateConfigs;

  HttpResponse _httpResponse;
  RequestHandler _requestHandler;
};
