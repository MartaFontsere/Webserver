/*
Este archivo se encarga de generar listados

✔ Concepto:
    Cuando accedes a una ruta que es un directorio sin index.html, y el
autoindex está activado en la config, el servidor debe devolver una página HTML
generada al vuelo que lista: Archivos, Directorios y Con enlaces navegables

🔥 Función final: generateListing(dirPath, urlPath)
    Se encarga de:
        Abrir el directorio
        Leer contenido
        Ordenarlo alfabéticamente (detalle elegante)
        Crear HTML escapado
        Generar enlaces correctos incluso con rutas con /

*/

#include "http/Autoindex.hpp"
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace Autoindex {

/**
 * @brief Genera el listado HTML de un directorio.
 *
 * Esta función genera una página HTML con el listado del contenido de un
 * directorio. Es lo que en Apache o Nginx se llama autoindex.
 *
 * @param dirPath Ruta real en el sistema de archivos.
 * @param urlPath Ruta URL solicitada por el cliente.
 * @return std::string El contenido HTML generado.
 */
std::string generateListing(const std::string &dirPath,
                            const std::string &urlPath) {
  // 1. Abrimos el directorio físico en el disco
  DIR *dir = opendir(dirPath.c_str());
  if (!dir) {
    // Si falla, el llamador (StaticFileHandler) decidirá qué error enviar
    // (403/404)
    return "";
  }
  // 2. Preparamos el buffer de texto para el HTML
  std::ostringstream html;

  // Escapar urlPath para HTML
  std::string safeUrlPath = escapeHtml(urlPath);

  // Cabecera HTML + CSS
  html << "<!DOCTYPE html>\n"
       << "<html>\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <title>Index of " << safeUrlPath << "</title>\n"
       << "  <style>\n"
       << "    body { font-family: Arial, sans-serif; margin: 40px; }\n"
       << "    h1 { color: #333; margin-bottom: 20px; }\n"
       << "    table { width: 100%; border-collapse: collapse; }\n"
       << "    th, td { text-align: left; padding: 8px; border-bottom: 1px "
          "solid #ddd; }\n"
       << "    th { background-color: #f2f2f2; font-weight: bold; }\n"
       << "    a { text-decoration: none; color: #0066cc; }\n"
       << "    a:hover { text-decoration: underline; }\n"
       << "    .size { text-align: right; }\n"
       << "    .dir { font-weight: bold; }\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <h1>Index of " << safeUrlPath << "</h1>\n"
       << "  <table>\n"
       << "    <tr>\n"
       << "      <th>Name</th>\n"
       << "      <th>Last Modified</th>\n"
       << "      <th class=\"size\">Size</th>\n"
       << "    </tr>\n";

  // 3. Generamos el enlace "../" para subir de nivel (si no estamos en raíz)
  if (urlPath != "/" && !urlPath.empty()) {
    // Encontrar directorio padre
    std::string parentPath = urlPath;
    if (parentPath[parentPath.size() - 1] == '/')
      parentPath.erase(parentPath.size() - 1);
    size_t lastSlash = parentPath.find_last_of('/');

    if (lastSlash == std::string::npos)
      parentPath = "/";
    else
      parentPath = parentPath.substr(0, lastSlash + 1);
    html << "    <tr>\n"
         << "      <td><a href=\"" << parentPath << "\">../</a></td>\n"
         << "      <td>-</td>\n"
         << "      <td class=\"size\">-</td>\n"
         << "    </tr>\n";
  }
  // 4. El bucle principal: leer entradas del directorio
  struct dirent *entry;
  int entryCount = 0;
  const int MAX_ENTRIES = 1000; // Límite por seguridad

  // Iterar por los archivos del directorio
  while ((entry = readdir(dir)) != NULL && entryCount < MAX_ENTRIES) {
    std::string name = entry->d_name;
    // Saltar . y .. (ya manejamos .. manualmente arriba)
    if (name == "." || name == "..")
      continue;

    // Construir path completo para poder hacer stat()
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/')
      fullPath += "/";
    fullPath += name;

    // 5. Obtener metadatos con stat() para obtener información del archivo
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) == 0) {
      bool isDirectory = S_ISDIR(fileStat.st_mode);

      // Formatear la fecha de modificación en formato legible
      char dateBuf[64];
      struct tm *timeinfo = localtime(&fileStat.st_mtime);
      if (timeinfo)
        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", timeinfo);
      else
        std::strcpy(dateBuf, "-");
      // TODO: REVISAR QUE SE PUEDAN USAR ESTAS FUNCIONES

      // 6. Formatear la fecha de modificación y el tamaño (Convertir bytes a
      // KB/MB)
      std::string sizeStr;
      if (isDirectory)
        sizeStr = "-";
      else {
        // Convertir bytes a KB/MB
        if (fileStat.st_size < 1024) {
          std::ostringstream oss;
          oss << fileStat.st_size << " B";
          sizeStr = oss.str();
        } else if (fileStat.st_size < 1024 * 1024) {
          std::ostringstream oss;
          oss << (fileStat.st_size / 1024) << " KB";
          sizeStr = oss.str();
        } else {
          std::ostringstream oss;
          oss << (fileStat.st_size / (1024 * 1024)) << " MB";
          sizeStr = oss.str();
        }
      }

      // Preparar nombre para mostrar -> Añadir '/' visual a directorios
      std::string displayName = escapeHtml(name);
      if (isDirectory)
        displayName += "/";

      // 7. Generar el enlace seguro y la URL codificada
      std::string urlEncodedName = urlEncode(name);

      html << "    <tr>\n"
           << "      <td class=\"" << (isDirectory ? "dir" : "") << "\">"
           << "<a href=\"" << urlEncodedName << (isDirectory ? "/" : "")
           << "\">" << displayName << "</a></td>\n"
           << "      <td>" << dateBuf << "</td>\n"
           << "      <td class=\"size\">" << sizeStr << "</td>\n"
           << "    </tr>\n";
      entryCount++;
    }
  }
  // 8. Limpieza final: cerramos el directorio
  closedir(dir);

  // Si llegamos al límite, mostrar advertencia
  if (entryCount >= MAX_ENTRIES) {
    html
        << "    <tr>\n"
        << "      <td colspan=\"3\" style=\"color: #666; font-style: italic;\">"
        << "(Showing first " << MAX_ENTRIES << " entries)</td>\n"
        << "    </tr>\n";
  }

  // Cerrar HTML
  html << "  </table>\n"
       << "  <hr>\n"
       << "  <p><em>webserv/1.0</em> - Autoindex</p>\n"
       << "</body>\n"
       << "</html>";

  return html.str();
}

/**
 * @brief Escapa caracteres especiales de HTML.
 *
 * Función que convierte caracteres peligrosos en entidades.
 * Esto evita XSS al mostrar nombres de archivos que contienen <script>.
 */
std::string escapeHtml(const std::string &input) {
  std::string output;
  output.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    switch (input[i]) {
    case '&':
      output.append("&amp;");
      break;
    case '<':
      output.append("&lt;");
      break;
    case '>':
      output.append("&gt;");
      break;
    case '"':
      output.append("&quot;");
      break;
    case '\'':
      output.append("&#39;");
      break;
    default:
      output.push_back(input[i]);
      break;
    }
  }
  return output;
}

/**
 * @brief Codifica un string para ser usado en una URL.
 *
 * Esta función hace lo contrario a urlDecode: recibe un string real y genera
 * una versión segura para URL. Necesario para poner links correctos en el
 * autoindex.
 *
 * Convierte nombres de archivo a formato URL seguro:
 * - letras/números -> iguales
 * - espacio -> %20
 * - caracteres raros -> %XX
 *
 * Ejemplo: My File#.txt -> My%20File%23.txt
 */
std::string urlEncode(const std::string &input) {
  std::ostringstream encoded;
  for (size_t i = 0; i < input.size(); ++i) {
    unsigned char c = input[i];

    // Caracteres seguros (RFC 3986)
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
    } else if (c == ' ') {
      // Como esta función solo se usa para autoindex / paths, el espacio
      // siempre es %20
      encoded << "%20";
    } else {
      /*
      Vamos bit a bit para generar el %XX.

      🧠 Recordatorio: ¿qué es un byte?
      Un unsigned char tiene 8 bits: [hhhh][llll]
          hhhh -> 4 bits altos (high nibble)
          llll -> 4 bits bajos (low nibble)

      (c >> 4) -> Desplaza 4 bits a la derecha para obtener el nibble alto.
      (c & 0x0F) -> Máscara para obtener los 4 bits bajos.
      */
      encoded << '%' << std::hex << std::uppercase << (int)(c >> 4)
              << (int)(c & 0x0F);
    }
  }
  return encoded.str();
}

} // namespace Autoindex

// TODO: REVISAR SI HAY QUE HACER UN urlEncodeQuery o algo asi