#pragma once

#include "config/LocationConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <map>
#include <string>

/**
 * @class StaticFileHandler
 * @brief Manejador especializado en recursos estáticos y sistema de archivos.
 *
 * Esta clase hereda la lógica que antes estaba dispersa en Client y Autoindex.
 * Se encarga de la interacción directa con el disco.
 *
 * Responsabilidades:
 * 1. Servir archivos: Lectura de archivos y determinación de tipos MIME.
 * 2. Autoindex: Generación dinámica de listados HTML para directorios.
 * 3. Uploads: Gestión de subida de archivos mediante POST.
 * 4. Deletes: Eliminación de recursos mediante DELETE.
 * 5. Seguridad: Saneamiento de rutas para prevenir Path Traversal.
 */
class StaticFileHandler {
public:
  StaticFileHandler();
  ~StaticFileHandler();

  /**
   * @brief Handles a GET request for a static file or directory.
   */
  void handleGet(const HttpRequest &request, HttpResponse &response,
                 const LocationConfig &location);

  /**
   * @brief Handles a POST request for file upload.
   */
  void handlePost(const HttpRequest &request, HttpResponse &response,
                  const LocationConfig &location);

  /**
   * @brief Handles a DELETE request for a file.
   */
  void handleDelete(const HttpRequest &request, HttpResponse &response,
                    const LocationConfig &location);

  /**
   * @brief Handles a HEAD request (same as GET but without body).
   */
  void handleHead(const HttpRequest &request, HttpResponse &response,
                  const LocationConfig &location);

  /**
   * @brief Sirve un archivo específico del disco al cliente.
   * @param fullPath Ruta absoluta en el sistema de archivos.
   * @param response Objeto respuesta donde se cargará el contenido.
   */
  void serveStaticFile(const std::string &fullPath, HttpResponse &response);

private:
  std::map<std::string, std::string> _mimeTypes;

  void _initMimeTypes();
  std::string _determineMimeType(const std::string &path);

  // Helpers moved from Client/Autoindex
  std::string _getDecodedPath(const std::string &rawPath) const;
  std::string _sanitizePath(const std::string &decodedPath) const;
  std::string _urlDecode(const std::string &encoded, bool plusAsSpace) const;
  bool _readFileToString(const std::string &fullPath, std::string &out,
                         size_t size);

  // Autoindex logic
  void _handleDirectory(const std::string &dirPath, const std::string &urlPath,
                        const LocationConfig &location, HttpResponse &response);

  static const size_t MAX_STATIC_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
};
