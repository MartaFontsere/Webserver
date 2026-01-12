#include "http/RequestHandler.hpp"
#include "cgi/CGIDetector.hpp"
#include "cgi/CGIHandler.hpp"
#include "network/ClientConnection.hpp" // For CGI async
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

/**
 * @brief Implementación del orquestador de peticiones.
 *
 * El flujo general es:
 * 1. Identificar el host virtual (ServerConfig).
 * 2. Identificar la ruta (LocationConfig).
 * 3. Validar si el método está permitido.
 * 4. Si es CGI, delegar al CGIHandler.
 * 5. Si es estático, delegar al StaticFileHandler.
 */
RequestHandler::RequestHandler() {}

RequestHandler::~RequestHandler() {}

HttpResponse
RequestHandler::handleRequest(const HttpRequest &request,
                              const std::vector<ServerConfig> &candidateConfigs,
                              ClientConnection *client) {
  // 0. Reseteamos cualquier HttpResponse previa (estado limpio) -> Limpia
  // HttpResponse previo
  HttpResponse response;

  // 1. Virtual Hosting: Match ServerConfig based on Host header
  // De todos los servers que escuchan en este puerto, buscamos el
  // que tenga el host que coincide con el host del request (el que
  // corresponda a este dominio). Guardamos la linea entera en un
  // string
  const ServerConfig *matchedConfig =
      _matchVirtualHost(request, candidateConfigs);
  if (!matchedConfig) {
    // This should theoretically not happen if candidateConfigs is not empty
    // We don't have a config yet, so we use a dummy one or just the default
    // error
    response.setErrorResponse(500);
    return response;
  }

  // 2. Location Matching
  const LocationConfig *matchedLocation =
      _matchLocation(request.getPath(), *matchedConfig);
  // Si no hay ninguna location que matchee, es un error de config
  // (nginx siempre tiene al menos "/")
  if (!matchedLocation) {
    std::cout << "[DEBUG] No location matched" << std::endl;
    _sendError(404, response, *matchedConfig, request);
    return response;
  }

  const LocationConfig &location =
      *matchedLocation; // matchedLocation es un puntero, por lo que lo
                        // igualamos al objeto LocationConfig que apunta el
                        // puntero.

  // 3. Check Method Allowed
  const std::string &method = request.getMethod();
  if (!location.isMethodAllowed(method)) {
    _sendError(405, response, *matchedConfig, request, &location);
    return response;
  }

  // 4. Check Client Max Body Size (default: 1MB, configured via
  // client_max_body_size en el config)
  if (request.getBody().size() > location.getMaxBodySize()) {
    _sendError(413, response, *matchedConfig, request, &location);
    return response;
  }

  // 5. Check for Redirects (return)
  if (location.getReturnCode() != 0) {
    response.setStatus(location.getReturnCode(), "Redirect");
    response.setHeader("Location", location.getReturnUrl());
    _applyConnectionHeader(request, response);
    return response;
  }

  // 6. Check CGI
  // Una vez sé QUÉ reglas aplican (LocationConfig), cómo ejecuto esta request?
  // si es CGI ejecutar programa
  // sino, servir recurso estático (GET, HEAD, POST...)
  if (CGIDetector::isCGIRequest(
          request.getPath(),
          location.getCgiExts())) // Este path (URL) corresponde a un CGI según
                                  // las reglas de esta location?
  {
    CGIHandler cgiHandler; // se crea un ejecutor de CGI
    std::string serverName = request.getOneHeader("Host"); // obtenemos el
    // nombre del server
    size_t colonPos = serverName.find(':'); // buscamos el puerto
    if (colonPos != std::string::npos)
      serverName = serverName.substr(0, colonPos); // quitamos el puerto

    if (serverName.empty() && !matchedConfig->getServerNames().empty()) {
      serverName = matchedConfig->getServerNames()[0]; // si esta vacio, usamos
                                                       // el nombre del server
    }
    int serverPort =
        matchedConfig->getListen(); // obtenemos el puerto del server

    // === CGI ASYNC PATH ===
    if (client) {
      // Async execution: fork now, read later via poll()
      CGIAsyncResult asyncResult =
          cgiHandler.handleAsync(request, location, serverName, serverPort);

      if (asyncResult.success) {
        // Start tracking CGI in client
        client->startCGI(asyncResult.pipeFd, asyncResult.childPid);
        // Mark response as pending - will be completed by Server::handleCGIPipe
        response.setCGIPending(true);
        return response;
      } else {
        // If async failed, return 500 Internal Server Error
        std::cerr << "[Error] CGI async execution failed" << std::endl;
        _sendError(500, response, *matchedConfig, request, &location);
        _applyConnectionHeader(request, response);
        return response;
      }
    }

    // Fallback: sync execution (when client is NULL, e.g. internal tests)
    response = cgiHandler.handle(request, location, serverName,
                                 serverPort); // Ejecutamos el CGI usando las
    // reglas de esta location
    _applyConnectionHeader(request, response); // Añadimos el header Connection
    return response;
  }

  // 7. Handle static file -> si no es CGI, se ejecuta el handler
  // correspondiente
  if (method == "GET") {
    _staticHandler.handleGet(request, response, location);
  } else if (method == "HEAD") {
    _staticHandler.handleHead(request, response, location);
  } else if (method == "POST") {
    _staticHandler.handlePost(request, response, location);
  } else if (method == "DELETE") {
    _staticHandler.handleDelete(request, response, location);
  } else {
    _sendError(405, response, *matchedConfig, request, &location);
  }

  // 8. Si el handler ha puesto un código de error, aplicamos páginas
  // personalizadas si existen
  if (response.getStatusCode() >= 400) {
    _sendError(response.getStatusCode(), response, *matchedConfig, request,
               &location);
  }

  _applyConnectionHeader(request, response);
  return response;
}

const ServerConfig *RequestHandler::_matchVirtualHost(
    const HttpRequest &request,
    const std::vector<ServerConfig> &candidateConfigs) {
  if (candidateConfigs.empty())
    return NULL;

  std::string host =
      request.getOneHeader("Host"); // Buscamos el host del request
  // Remove port from host if present
  size_t colonPos = host.find(':');
  if (colonPos != std::string::npos)
    host = host.substr(0, colonPos); // guardamos lo que hay antes de :,
                                     // quitando el puerto

  for (size_t i = 0; i < candidateConfigs.size();
       ++i) // Buscamos qué config coincide con el host
  {
    const std::vector<std::string> &serverNames =
        candidateConfigs[i].getServerNames(); // Obtenemos los nombres de
    // los servers candidatos
    for (size_t j = 0; j < serverNames.size(); ++j) {
      if (serverNames[j] ==
          host) // Si alguno coincide, devolvemos la config concreta
      {
        return &candidateConfigs[i];
      }
    }
  }
  return &candidateConfigs[0]; // Default to first config
}

const LocationConfig *
RequestHandler::_matchLocation(const std::string &path,
                               const ServerConfig &config) {
  const std::vector<LocationConfig> &locations = config.getLocations();
  // Obtenemos las locations de la config seleccionada
  const LocationConfig *matchedLocation = NULL; // Inicializamos a NULL
  size_t longestMatch =
      0; // Inicializamos la longitud de la location seleccionada

  for (size_t i = 0; i < locations.size(); ++i) {
    const std::string &locationPattern = locations[i].getPattern();
    // Obtenemos el path de la location
    // Lo llamamos pattern para evitar confusiones, porque en un servidor la
    // palabra "path" es ambigua. Puede referirse a La URL que pide el cliente
    // (ej: /images/gato.jpg), o la ruta en el disco duro (ej:
    // /var/www/html/images/gato.jpg). Al usar getPattern(), dejamos claro que
    // nos referimos al patrón de coincidencia definido en el archivo de
    // configuración (el prefijo location /images { ... }), es decir que estamos
    // obteniendo la regla de la URL (el patrón) y no una ruta de archivo en el
    // disco

    // ¿El path del request empieza por el pattern de la location? Lo primero es
    // el dato, y lo segundo la regla, miramos si coincid
    if (path.compare(0, locationPattern.length(), locationPattern) == 0) {
      // Nos quedamos con la location más específica (path más largo)
      if (locationPattern.length() > longestMatch) {
        longestMatch = locationPattern.length();
        matchedLocation = &locations[i];
      }
    }
  }
  return matchedLocation;
}

void RequestHandler::_applyConnectionHeader(const HttpRequest &request,
                                            HttpResponse &response) {
  if (request.isKeepAlive())
    response.setHeader("Connection", "keep-alive");
  else
    response.setHeader("Connection", "close");
}

void RequestHandler::_sendError(int errorCode, HttpResponse &response,
                                const ServerConfig &config,
                                const HttpRequest &request,
                                const LocationConfig *location) {
  // --- PASO 1: Búsqueda jerárquica de la página de error ---
  // El objetivo es encontrar una ruta de archivo (ej: "error_pages/404.html")
  // definida en la configuración para este código de error específico.

  std::string errorPagePath;
  std::string rootUsed;

  // Prioridad 1: Buscar en la Location actual (regla más específica)
  if (location) {
    const std::map<int, std::string> &locErrorPages = location->getErrorPages();
    std::map<int, std::string>::const_iterator it =
        locErrorPages.find(errorCode);
    if (it != locErrorPages.end()) {
      errorPagePath = it->second;
      rootUsed = location->getRoot(); // Si la página es de la location, usamos
                                      // su root para resolver la ruta relativa
    }
  }

  // Prioridad 2: Si no se encontró en la Location, buscar en el Server (regla
  // general)
  if (errorPagePath.empty()) {
    const std::map<int, std::string> &servErrorPages = config.getErrorPages();
    std::map<int, std::string>::const_iterator it =
        servErrorPages.find(errorCode);
    if (it != servErrorPages.end()) {
      errorPagePath = it->second;
      rootUsed = config.getRoot(); // Si la página es del server, usamos el root
                                   // del server
    }
  }

  // --- PASO 2: Intento de carga del archivo personalizado - Construcción de la
  // ruta y lectura del archivo---
  // Si hemos encontrado una ruta en la configuración, intentamos leerla del
  // disco.
  if (!errorPagePath.empty()) {
    // Si no hay root definido, usamos el directorio actual
    if (rootUsed.empty())
      rootUsed = ".";
    // Limpiamos la barra final del root si la tiene
    if (rootUsed[rootUsed.size() - 1] == '/')
      rootUsed.erase(rootUsed.size() - 1);

    // Aseguramos que la ruta del error empiece por / para concatenar bien
    std::string separator = (errorPagePath[0] == '/') ? "" : "/";
    std::string fullPath = rootUsed + separator + errorPagePath;

    // Intentamos abrir y leer el archivo
    struct stat fileStat;
    // 1. ¿Existe y es un archivo regular?
    if (stat(fullPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
      std::ifstream file(fullPath.c_str());
      if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();

        // Si la lectura es exitosa, configuramos la respuesta con el contenido
        // del archivo
        response.setStatus(errorCode, "Error"); // El mensaje se puede refinar
        response.setHeader("Content-Type", "text/html");
        response.setBody(buffer.str());
        _applyConnectionHeader(request, response);
        return; // Error personalizado enviado con éxito
      }
    }
    // Si el archivo no existe o no se puede leer, el flujo continúa al Paso 3
  }

  // --- PASO 3: Fallback (Error por defecto) ---
  // Si no hay página personalizada definida, o si el archivo no se pudo leer,
  // generamos una respuesta de error genérica (hardcoded en HttpResponse).
  response.setErrorResponse(errorCode);
  _applyConnectionHeader(request, response);
}
