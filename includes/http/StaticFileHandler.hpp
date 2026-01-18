#pragma once

#include "config/LocationConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <map>
#include <string>

/**
 * @brief Static file and directory handler - serves files, autoindex, uploads
 */
class StaticFileHandler {
public:
  StaticFileHandler();
  ~StaticFileHandler();

  /** @brief Handle GET request for file or directory */
  void handleGet(const HttpRequest &request, HttpResponse &response,
                 const LocationConfig &location);

  /** @brief Handle POST request for file upload */
  void handlePost(const HttpRequest &request, HttpResponse &response,
                  const LocationConfig &location);

  /** @brief Handle DELETE request */
  void handleDelete(const HttpRequest &request, HttpResponse &response,
                    const LocationConfig &location);

  /** @brief Handle HEAD request (GET without body) */
  void handleHead(const HttpRequest &request, HttpResponse &response,
                  const LocationConfig &location);

  /** @brief Serve a specific file from disk */
  void serveStaticFile(const std::string &fullPath, HttpResponse &response);

private:
  std::map<std::string, std::string> _mimeTypes;

  void _initMimeTypes();
  std::string _determineMimeType(const std::string &path);
  std::string _sanitizePath(const std::string &decodedPath) const;
  bool _readFileToString(const std::string &fullPath, std::string &out,
                         size_t size);
  void _handleDirectory(const std::string &dirPath, const std::string &urlPath,
                        const LocationConfig &location, HttpResponse &response);

  static const size_t MAX_STATIC_FILE_SIZE = 10 * 1024 * 1024;
};
