#pragma once

#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/StaticFileHandler.hpp"
#include <vector>

/**
 * La clase RequestHandler es el Orquestador de la capa HTTP.
 *
 * Esta clase es el "cerebro" del procesamiento de peticiones, es decir, decide
 * qué hacer con la petición.
 *
 * Responsabilidades:
 * 1. Virtual Hosting: Seleccionar el ServerConfig correcto basado en el header
 * 'Host'.
 * 2. Location Matching: Encontrar la configuración de ruta más específica.
 * 3. Validación: Comprobar métodos permitidos, límites de tamaño y
 * redirecciones.
 * 4. Despacho: Decidir si la petición va a CGI o al StaticFileHandler.
 */
class RequestHandler {
public:
  RequestHandler();
  ~RequestHandler();

  /**
   * @brief Punto de entrada principal para el procesamiento de una petición.
   *
   * @param request La petición HTTP completa.
   * @param candidateConfigs Lista de ServerConfigs que coinciden con el puerto.
   * @return HttpResponse La respuesta generada.
   */
  HttpResponse handleRequest(const HttpRequest &request,
                             const std::vector<ServerConfig> &candidateConfigs);

private:
  StaticFileHandler _staticHandler;

  // Helper methods for the logic flow
  const ServerConfig *
  _matchVirtualHost(const HttpRequest &request,
                    const std::vector<ServerConfig> &candidateConfigs);
  const LocationConfig *_matchLocation(const std::string &path,
                                       const ServerConfig &config);

  void _applyConnectionHeader(const HttpRequest &request,
                              HttpResponse &response);
  void _sendError(int errorCode, HttpResponse &response,
                  const ServerConfig &config, const HttpRequest &request,
                  const LocationConfig *location = NULL);
};
