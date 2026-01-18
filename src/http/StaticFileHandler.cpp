#include "http/StaticFileHandler.hpp"
#include "http/Autoindex.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @file StaticFileHandler.cpp
 * @brief Static file serving, directory handling, uploads, and deletes
 *
 * This module handles all filesystem interactions for the web server:
 * - GET: Serve files and directory listings
 * - HEAD: Return headers without body
 * - POST: Handle file uploads
 * - DELETE: Remove files
 *
 * Key features:
 * - MIME type detection by file extension
 * - Path traversal protection via sanitization
 * - Root/Alias path resolution (Nginx-style)
 * - Autoindex directory listing
 * - Secure file operations (O_NOFOLLOW, fsync)
 *
 * @see Autoindex for directory listing generation
 * @see RequestHandler for routing to this handler
 */

/**
 * @brief Constructor - initializes MIME type mappings
 */
StaticFileHandler::StaticFileHandler() { _initMimeTypes(); }

/**
 * @brief Destructor
 */
StaticFileHandler::~StaticFileHandler() {}

/**
 * @brief Initializes the MIME type lookup table
 */
void StaticFileHandler::_initMimeTypes() {
  _mimeTypes["html"] = "text/html";
  _mimeTypes["css"] = "text/css";
  _mimeTypes["js"] = "application/javascript";
  _mimeTypes["png"] = "image/png";
  _mimeTypes["jpg"] = "image/jpeg";
  _mimeTypes["jpeg"] = "image/jpeg";
  _mimeTypes["gif"] = "image/gif";
  _mimeTypes["svg"] = "image/svg+xml";
  _mimeTypes["ico"] = "image/x-icon";
  _mimeTypes["txt"] = "text/plain";
  _mimeTypes["json"] = "application/json";
  _mimeTypes["pdf"] = "application/pdf";
}

/**
 * @brief Determines MIME type from file extension
 *
 * @param path File path
 * @return MIME type string, or "application/octet-stream" if unknown
 */
std::string StaticFileHandler::_determineMimeType(const std::string &path) {
  size_t dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos)
    return "application/octet-stream";

  std::string ext = path.substr(dotPos + 1);
  std::map<std::string, std::string>::iterator it = _mimeTypes.find(ext);
  if (it != _mimeTypes.end())
    return it->second;
  return "application/octet-stream";
}

/**
 * @brief Reads file content into a string using low-level I/O
 *
 * @param fullPath Absolute path to file
 * @param out Output string to store content
 * @param size Expected file size
 * @return true on success, false on error
 *
 * @note Uses O_NOFOLLOW for security (don't follow symlinks)
 * @note Handles EINTR interruptions by retrying
 */
bool StaticFileHandler::_readFileToString(const std::string &fullPath,
                                          std::string &out, size_t size) {
  int flags = O_RDONLY;
#ifdef O_NOFOLLOW
  flags |= O_NOFOLLOW; // Security: don't follow symlinks
#endif

  int fd = open(fullPath.c_str(), flags);
  if (fd < 0) {
    return false;
  }

  out.clear();
  try {
    out.resize(size);
  } catch (const std::exception &e) {
    close(fd);
    return false;
  }

  size_t total = 0;
  while (total < size) {
    ssize_t bytesRead = read(fd, &out[total], size - total);
    if (bytesRead < 0) {
      if (errno == EINTR)
        continue; // Retry if interrupted by signal
      close(fd);
      return false;
    }
    if (bytesRead == 0) {
      break; // EOF
    }
    total += static_cast<size_t>(bytesRead);
  }

  close(fd);

  if (total < size)
    out.resize(total);

  return true;
}

/**
 * @brief Sanitizes URL path to prevent path traversal attacks
 *
 * Handles:
 * - Empty paths → "/"
 * - Paths not starting with "/" → "__FORBIDDEN__"
 * - ".." segments that escape root → "__FORBIDDEN__"
 * - "." segments → ignored
 * - Trailing slashes preserved
 *
 * @param decodedPath URL-decoded path from request
 * @return Sanitized path, or "__FORBIDDEN__" if invalid
 */
std::string
StaticFileHandler::_sanitizePath(const std::string &decodedPath) const {
  if (decodedPath.empty())
    return "/";

  if (decodedPath[0] != '/')
    return "__FORBIDDEN__";

  std::vector<std::string> allParts;
  bool endsWithSlash =
      (decodedPath.size() > 1 && decodedPath[decodedPath.size() - 1] == '/');

  size_t i = 1;
  while (i <= decodedPath.size()) {
    size_t j = decodedPath.find('/', i);
    std::string part;
    if (j == std::string::npos) {
      part = decodedPath.substr(i);
      i = decodedPath.size() + 1;
    } else {
      part = decodedPath.substr(i, j - i);
      i = j + 1;
    }
    if (part.empty() || part == ".") {
      continue;
    } else if (part == "..") {
      if (allParts.empty()) {
        return "__FORBIDDEN__"; // Attempt to escape root
      }
      allParts.pop_back();
    } else
      allParts.push_back(part);
  }

  std::string cleanPath = "/";
  for (size_t k = 0; k < allParts.size(); ++k) {
    cleanPath += allParts[k];
    if (k + 1 < allParts.size())
      cleanPath += "/";
  }

  if (endsWithSlash && cleanPath[cleanPath.size() - 1] != '/')
    cleanPath += "/";

  return cleanPath;
}

/**
 * @brief Handles GET requests for static resources
 *
 * Flow:
 * 1. Sanitize path (prevent traversal)
 * 2. Build full filesystem path using root or alias
 * 3. Check file/directory existence with stat()
 * 4. If directory → delegate to _handleDirectory()
 * 5. If file → serve with serveStaticFile()
 *
 * @param request HTTP request object
 * @param response HTTP response object to populate
 * @param location Location configuration with root/alias settings
 */
void StaticFileHandler::handleGet(const HttpRequest &request,
                                  HttpResponse &response,
                                  const LocationConfig &location) {
  std::string decodedPath = request.getPath();

  std::string cleanPath = _sanitizePath(decodedPath);
  if (cleanPath == "__FORBIDDEN__") {
    std::cerr << "❌ [Error] Path forbidden by sanitization: " << decodedPath
              << std::endl;
    response.setErrorResponse(403);
    return;
  }

  std::cout << "[Info] GET request path: " << decodedPath << std::endl;

  // Build full path (Nginx-style root/alias logic)
  std::string fullPath;
  if (location.hasAlias()) {
    // ALIAS: Replace location pattern with alias path
    std::string relativePath = cleanPath.substr(location.getPattern().size());
    if (relativePath.empty() || relativePath[0] != '/')
      relativePath = "/" + relativePath;

    std::string aliasPath = location.getAlias();
    if (!aliasPath.empty() && aliasPath[aliasPath.size() - 1] == '/')
      aliasPath.erase(aliasPath.size() - 1);

    fullPath = aliasPath + relativePath;
    std::cout << "[Debug] Using ALIAS: " << fullPath << std::endl;
  } else {
    // ROOT: Append path to root directory
    std::string rootPath = location.getRoot();
    if (!rootPath.empty() && rootPath[rootPath.size() - 1] == '/')
      rootPath.erase(rootPath.size() - 1);

    fullPath = rootPath + cleanPath;
    std::cout << "[Debug] Using ROOT: " << fullPath << std::endl;
  }

  std::cout << "[Info] Full filesystem path: " << fullPath << std::endl;

  // Check existence with stat()
  struct stat fileStat;
  if (stat(fullPath.c_str(), &fileStat) != 0) {
    if (errno == EACCES) {
      std::cerr << "❌ [Error] Access denied: " << fullPath << std::endl;
      response.setErrorResponse(403);
    } else {
      std::cerr << "❌ [Error] Not found: " << fullPath << std::endl;
      response.setErrorResponse(404);
    }
    return;
  }

  // Handle directory
  if (S_ISDIR(fileStat.st_mode)) {
    std::cout << "[Debug] Directory detected → handling autoindex/index"
              << std::endl;
    _handleDirectory(fullPath, decodedPath, location, response);
    return;
  }

  // Serve file
  serveStaticFile(fullPath, response);
}

/**
 * @brief Handles HEAD requests (GET without body)
 *
 * @param request HTTP request
 * @param response HTTP response
 * @param location Location configuration
 */
void StaticFileHandler::handleHead(const HttpRequest &request,
                                   HttpResponse &response,
                                   const LocationConfig &location) {
  handleGet(request, response, location);
  response.setBody(""); // Remove body, keep headers
}

/**
 * @brief Serves a static file
 *
 * @param fullPath Absolute filesystem path
 * @param response HTTP response to populate
 */
void StaticFileHandler::serveStaticFile(const std::string &fullPath,
                                        HttpResponse &response) {
  if (fullPath == "__FORBIDDEN__") {
    std::cerr << "❌ [Error] Path forbidden: " << fullPath << std::endl;
    response.setErrorResponse(403);
    return;
  }

  struct stat fileStat;
  if (stat(fullPath.c_str(), &fileStat) != 0) {
    if (errno == EACCES) {
      std::cerr << "❌ [Error] Access denied: " << fullPath << std::endl;
      response.setErrorResponse(403);
    } else {
      std::cerr << "❌ [Error] Not found: " << fullPath << std::endl;
      response.setErrorResponse(404);
    }
    return;
  }

  if (fileStat.st_size < 0) {
    std::cerr << "❌ [Error] Invalid file size: " << fullPath << std::endl;
    response.setErrorResponse(500);
    return;
  }

  size_t size = static_cast<size_t>(fileStat.st_size);
  if (size > MAX_STATIC_FILE_SIZE) {
    std::cerr << "❌ [Error] File too large (" << size << " bytes): " << fullPath
              << std::endl;
    response.setErrorResponse(413);
    return;
  }

  std::string content;
  if (!_readFileToString(fullPath, content, size)) {
    if (errno == EACCES) {
      std::cerr << "❌ [Error] Read permission denied: " << fullPath
                << std::endl;
      response.setErrorResponse(403);
    } else if (errno == ENOENT) {
      std::cerr << "❌ [Error] File disappeared: " << fullPath << std::endl;
      response.setErrorResponse(404);
    } else if (errno == EFBIG) {
      std::cerr << "❌ [Error] File too large: " << fullPath << std::endl;
      response.setErrorResponse(413);
    } else {
      std::cerr << "❌ [Error] Read failed: " << fullPath << " ("
                << strerror(errno) << ")" << std::endl;
      response.setErrorResponse(500);
    }
    return;
  }

  std::string mime = _determineMimeType(fullPath);

  std::ostringstream oss;
  oss << content.size();

  response.setStatus(200, "OK");
  response.setHeader("Content-Type", mime);
  response.setHeader("Content-Length", oss.str());
  response.setBody(content);

  std::cout << "✅ [Info] File served: " << fullPath << "\n";
}

/**
 * @brief Handles directory requests (index file or autoindex)
 *
 * Priority:
 * 1. Serve index file (e.g., index.html) if exists
 * 2. Generate autoindex listing if enabled
 * 3. Return 403 Forbidden otherwise
 *
 * @param dirPath Filesystem directory path
 * @param urlPath URL path for links
 * @param location Location configuration
 * @param response HTTP response to populate
 */
void StaticFileHandler::_handleDirectory(const std::string &dirPath,
                                         const std::string &urlPath,
                                         const LocationConfig &location,
                                         HttpResponse &response) {
  bool autoindexEnabled = location.getAutoindex();
  std::string defaultFile =
      location.getIndex().empty() ? "" : location.getIndex()[0];

  std::cout << "[Debug] handleDirectory: " << dirPath
            << ", autoindex=" << (autoindexEnabled ? "ON" : "OFF")
            << ", index=" << defaultFile << std::endl;

  // Priority 1: Try to serve index file
  std::string indexPath = dirPath;
  if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
    indexPath += "/";
  indexPath += defaultFile;

  struct stat fileStat;
  if (!defaultFile.empty() && stat(indexPath.c_str(), &fileStat) == 0 &&
      S_ISREG(fileStat.st_mode)) {
    std::cout << "[Debug] Serving index: " << indexPath << std::endl;
    serveStaticFile(indexPath, response);
    return;
  }
  std::cout << "[Debug] No index file found: " << indexPath << std::endl;

  // Priority 2: Generate autoindex if enabled
  if (autoindexEnabled) {
    std::cout << "[Debug] Generating autoindex for: " << dirPath << std::endl;
    std::string html = Autoindex::generateListing(dirPath, urlPath);
    if (html.empty()) {
      if (errno == EACCES) {
        std::cerr << "❌ [Error] Autoindex: permission denied: " << dirPath
                  << std::endl;
        response.setErrorResponse(403);
      } else {
        std::cerr << "❌ [Error] Autoindex failed: " << dirPath << std::endl;
        response.setErrorResponse(404);
      }
      return;
    }
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/html");
    response.setBody(html);
    return;
  }

  // Priority 3: No index and autoindex off → 403
  std::cout << "[Debug] No index, autoindex OFF → 403 Forbidden" << std::endl;
  response.setErrorResponse(403);
}

/**
 * @brief Handles POST requests (file uploads)
 *
 * Flow:
 * 1. Reject chunked uploads (not supported)
 * 2. Get upload directory from location config
 * 3. Create directory if needed
 * 4. Generate unique filename
 * 5. Write request body to file
 * 6. Respond with 201 Created
 *
 * @param request HTTP request with body
 * @param response HTTP response to populate
 * @param location Location configuration with upload_path
 */
void StaticFileHandler::handlePost(const HttpRequest &request,
                                   HttpResponse &response,
                                   const LocationConfig &location) {
  // Step 1: Reject chunked uploads
  if (request.isChunked()) {
    response.setStatus(501, "Not Implemented");
    response.setHeader("Content-Type", "text/html");
    response.setBody("<html><body><h1>501 Not Implemented</h1>"
                     "<p>Chunked uploads are not supported.</p>"
                     "</body></html>");
    return;
  }

  // Step 2: Get upload directory
  std::string uploadDir = location.getUploadPath();
  if (uploadDir.empty()) {
    std::cerr << "❌ [Error] No upload_path configured" << std::endl;
    response.setErrorResponse(500);
    return;
  }

  // Step 3: Verify/create upload directory
  struct stat fileStat;
  if (stat(uploadDir.c_str(), &fileStat) != 0) {
    if (errno == ENOENT) {
      if (mkdir(uploadDir.c_str(), 0755) != 0) {
        std::cerr << "❌ [Error] Failed to create upload directory: "
                  << uploadDir << std::endl;
        response.setErrorResponse(500);
        return;
      }
    } else {
      std::cerr << "❌ [Error] stat() failed on upload directory: " << uploadDir
                << std::endl;
      response.setErrorResponse(500);
      return;
    }
  } else if (!S_ISDIR(fileStat.st_mode)) {
    std::cerr << "❌ [Error] Upload path is not a directory: " << uploadDir
              << std::endl;
    response.setErrorResponse(500);
    return;
  }

  // Check write permission
  if (access(uploadDir.c_str(), W_OK) != 0) {
    std::cerr << "❌ [Error] Write permission denied: " << uploadDir
              << std::endl;
    response.setErrorResponse(403);
    return;
  }

  // Step 4: Generate unique filename
  std::ostringstream ss;
  time_t now = time(NULL);
  pid_t pid = getpid();
  int rnd = rand();

  ss << "upload_" << now << "_" << pid << "_" << rnd << ".dat";

  std::string filename = ss.str();
  std::string filepath = uploadDir;
  if (!filepath.empty() && filepath[filepath.size() - 1] != '/')
    filepath += "/";
  filepath += filename;

  // Step 5: Write file
  int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd == -1) {
    std::cerr << "❌ [Error] Failed to create upload file: " << filepath
              << std::endl;
    response.setErrorResponse(500);
    return;
  }

  const std::string &body = request.getBody();
  const char *buf = body.data();
  size_t buf_size = body.size();
  size_t written = 0;

  while (written < buf_size) {
    ssize_t ret = write(fd, buf + written, buf_size - written);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      std::cerr << "❌ [Error] Write failed: " << filepath << std::endl;
      close(fd);
      unlink(filepath.c_str()); // Clean up incomplete file
      response.setErrorResponse(500);
      return;
    }
    written += static_cast<size_t>(ret);
  }

  fsync(fd);
  close(fd);

  // Step 6: Respond with 201 Created
  response.setStatus(201, "Created");
  response.setHeader("Content-Type", "text/html");
  response.setHeader("Location", "/uploads/" + filename);

  std::ostringstream html;
  html << "<html><body>"
       << "<h1>Upload successful</h1>"
       << "<p>Saved as: " << filename << " (" << body.size() << " bytes)</p>"
       << "</body></html>";

  response.setBody(html.str());

  std::cout << "✅ [Info] Upload OK: " << filename << " (" << body.size()
            << " bytes)" << std::endl;
}

/**
 * @brief Handles DELETE requests
 *
 * Flow:
 * 1. Sanitize path
 * 2. Build full filesystem path
 * 3. Verify file exists and is not a directory
 * 4. Check write permission on parent directory
 * 5. Remove file
 * 6. Respond with 204 No Content
 *
 * @param request HTTP request
 * @param response HTTP response to populate
 * @param location Location configuration
 */
void StaticFileHandler::handleDelete(const HttpRequest &request,
                                     HttpResponse &response,
                                     const LocationConfig &location) {
  std::string decodedPath = request.getPath();
  std::string cleanPath = _sanitizePath(decodedPath);

  if (cleanPath == "__FORBIDDEN__") {
    response.setErrorResponse(403);
    return;
  }

  // Build full path (same logic as GET)
  std::string fullPath;
  if (location.hasAlias()) {
    std::string relativePath = cleanPath.substr(location.getPattern().size());
    if (relativePath.empty() || relativePath[0] != '/')
      relativePath = "/" + relativePath;

    std::string aliasPath = location.getAlias();
    if (!aliasPath.empty() && aliasPath[aliasPath.size() - 1] == '/')
      aliasPath.erase(aliasPath.size() - 1);

    fullPath = aliasPath + relativePath;
  } else {
    std::string rootPath = location.getRoot();
    if (!rootPath.empty() && rootPath[rootPath.size() - 1] == '/')
      rootPath.erase(rootPath.size() - 1);

    fullPath = rootPath + cleanPath;
  }

  std::cout << "[Debug] DELETE path: " << fullPath << std::endl;

  // Verify file exists
  struct stat fileStat;
  if (stat(fullPath.c_str(), &fileStat) != 0) {
    if (errno == ENOENT) {
      std::cerr << "❌ [Error] File not found: " << fullPath << std::endl;
      response.setErrorResponse(404);
    } else if (errno == EACCES) {
      std::cerr << "❌ [Error] Permission denied: " << fullPath << std::endl;
      response.setErrorResponse(403);
    } else {
      std::cerr << "❌ [Error] stat() failed: " << fullPath << std::endl;
      response.setErrorResponse(500);
    }
    return;
  }

  // Don't allow deleting directories
  if (S_ISDIR(fileStat.st_mode)) {
    std::cerr << "❌ [Error] Cannot delete directory: " << fullPath << std::endl;
    response.setErrorResponse(403);
    return;
  }

  // Check write permission on parent directory
  std::string parentDir = fullPath.substr(0, fullPath.find_last_of('/'));
  if (parentDir.empty())
    parentDir = ".";

  if (access(parentDir.c_str(), W_OK) != 0) {
    std::cerr << "❌ [Error] No write permission in: " << parentDir << std::endl;
    response.setErrorResponse(403);
    return;
  }

  // Delete file
  if (std::remove(fullPath.c_str()) != 0) {
    if (errno == EACCES || errno == EPERM) {
      std::cerr << "❌ [Error] Permission denied: " << fullPath << std::endl;
      response.setErrorResponse(403);
    } else {
      std::cerr << "❌ [Error] Remove failed: " << fullPath << std::endl;
      response.setErrorResponse(500);
    }
    return;
  }

  // Respond with 204 No Content
  response.setStatus(204, "No Content");
  response.setBody("");

  std::cout << "✅ [Info] File deleted: " << fullPath << std::endl;
}
