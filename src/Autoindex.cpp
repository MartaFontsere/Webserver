// /*
// Este archivo se encarga de generar listados

// ✔ Concepto:
//     Cuando accedes a una ruta que es un directorio sin index.html, y el autoindex está activado en la config, el servidor debe devolver una página HTML generada al vuelo que lista:
//         Archivos
//         Directorios
//         Con enlaces navegables

// 🔥 Función final: generateAutoIndex(dirPath, urlPath)
//     Se encarga de:
//         Abrir el directorio
//         Leer contenido
//         Ordenarlo alfabéticamente (detalle elegante)
//         Crear HTML escapado
//         Generar enlaces correctos incluso con rutas con /

// */

// #include "Autoindex.hpp"
// #include "Client.hpp"
// #include <dirent.h>   //opendir/readdir/closedir
// #include <sys/stat.h> //stat
// #include <sstream>    //std::ostringstream
// #include <iomanip>
// #include <cstdlib>
// #include <ctime>
// #include <iostream>

// namespace Autoindex
// {
//     // ============== FUNCIONES HELPER DE SEGURIDAD ==============

//     std::string escapeHtml(const std::string &input)
//     {
//         std::string output;
//         output.reserve(input.size());

//         for (size_t i = 0; i < input.size(); ++i)
//         {
//             switch (input[i])
//             {
//             case '&':
//                 output.append("&amp;");
//                 break;
//             case '<':
//                 output.append("&lt;");
//                 break;
//             case '>':
//                 output.append("&gt;");
//                 break;
//             case '"':
//                 output.append("&quot;");
//                 break;
//             case '\'':
//                 output.append("&#39;");
//                 break;
//             default:
//                 output.push_back(input[i]);
//                 break;
//             }
//         }
//         return output;
//     }

//     /*
//     Función que convierte caracteres peligrosos en entidades
//     Esto evita XSS al mostrar nombres de archivos que contienen <script>
//     TODO: DESARROLLAR MAS LA EXPLICACION
//     */

//     std::string urlEncode(const std::string &input)
//     {
//         std::ostringstream encoded;

//         for (size_t i = 0; i < input.size(); ++i)
//         {
//             unsigned char c = input[i];

//             // Caracteres seguros (solo estos carácteres pueden ir sin codificar, siguiendo el estándar RFC)
//             if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
//             {
//                 encoded << c;
//             }
//             // Espacios deben ser %20 en rutas
//             else if (c == ' ')
//             {
//                 encoded << "%20";
//             }
//             // Otros → %XX
//             else
//             {
//                 encoded << '%' << std::hex << std::uppercase
//                         << (int)(c >> 4) << (int)(c & 0x0F);
//             }
//         }
//         return encoded.str();
//     }

//     /*
//     Esta función hace lo contrario a urlDecode:
//         recibe un string real
//         genera una versión segura para URL

//     Necesario para tus autoindex, para poner links correctos.

//     Convierte nombres de archivo a formato URL seguro:
//         letras → iguales
//         espacio → +
//         caracteres raros → %XX

//     Ej: My File#.txt → My+File%23.txt
//     Así se mostraran enlaces que funcionen

//     Como esta función solo se usa para autoindex / paths, el espacio siempre es %20, no hay que distinguir con query ni usar el +

// CÓDIGO:
//     else
//     {
//         encoded << '%' << std::hex << std::uppercase
//                 << (int)(c >> 4) << (int)(c & 0x0F);
//     }

//     Vamos bit a bit.

//     🧠 Recordatorio: ¿qué es un byte?
//     Un unsigned char tiene 8 bits:
//         [hhhh][llll]
//             hhhh → 4 bits altos (high nibble)
//             llll → 4 bits bajos (low nibble)

//     Ejemplo:
//         char c = ' '; // espacio
//         ASCII = 32 decimal = 0x20 hex
//         Binario = 0010 0000

//     (c >> 4)
//         Desplaza 4 bits a la derecha:
//             0010 0000 >> 4 = 0000 0010

//         → 2 en decimal
//         → '2' en hex

//     (c & 0x0F)
//         Máscara para quedarte con los 4 bits bajos:
//             0010 0000
//             0000 1111
//             ---------
//             0000 0000

//         → 0

//     Resultado final
//         encoded << '%' << '2' << '0';
//     ✔ %20

//     TODO: REVISAR SI HAY QUE HACER UN urlEncodeQuery o algo asi
//     */

//     void handleDirectory(Client *client, const std::string &dirPath, const std::string &urlPath, bool autoindexEnabled, const std::string &defaultFile)
//     {
//         std::cout << "[DEBUG] handleDirectory: " << dirPath
//                   << ", autoindex=" << (autoindexEnabled ? "ON" : "OFF")
//                   << ", defaultFile=" << defaultFile << std::endl;

//         // 1) Intentar servir archivo index
//         std::string indexPath = dirPath;
//         if (dirPath[dirPath.size() - 1] != '/')
//         {
//             indexPath += "/";
//         }
//         indexPath += defaultFile;
//         // TODO: revisar si dirPath acaba en '/', no haria falta.

//         struct stat st;
//         if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
//         {
//             std::cout << "[DEBUG] Encontrado index: " << indexPath << std::endl;
//             client->serveStaticFile(indexPath); // Delegar en el método existente de Client
//             return;
//         }
//         std::cout << "[DEBUG] No encontrado index en: " << indexPath << std::endl;

//         // 2) Autoindex activado → generar listado
//         if (autoindexEnabled)
//         {
//             std::cout << "[DEBUG] Autoindex ON → Generando autoindex para: " << dirPath << std::endl;
//             generateListing(client, dirPath, urlPath);
//             return;
//         }

//         // 3) Directorio sin index y sin autoindex → 403 Forbidden
//         std::cout << "[DEBUG] Sin index y autoindex OFF → 403 Forbidden" << std::endl;
//         client->sendError(403);
//         return;
//         // TODO general:crear la funcion sendEerror(403, por ejemplo), y que ahi se llame a setErrorResponse y aplyConnectionHeader.
//     }

//     /*
//     Su propósito es: Decidir qué hacer cuando la ruta solicitada por el cliente apunta a un directorio.

//     Cuando el servidor recibe una petición GET, tú determinas si el path real del sistema es:
//         un archivo ✔️ → se sirve
//         un directorio 📁 → debes decidir qué hacer
//         inexistente ❌ → error

//     Cuando es un directorio, tienes 3 opciones:
//         Servir un archivo index.html (si existe)
//         Mostrar un listado de ficheros (autoindex on)
//         Devolver 403 Forbidden (si no hay index y autoindex está off)

//     handleDirectory() implementa justamente este flujo.

//     Por lo tanto, esta función decide si servir un defaultFile (p. ej. index.html) o generar el listado si autoindexEnabled == true. En caso contrario, envía error 403.

//     La llama tu manejador principal de rutas en el servidor
//     Cuando haces:
//         struct stat st;
//         stat(realPath.c_str(), &st);

//         if (S_ISDIR(st.st_mode))
//         {
//             return Autoindex::handleDirectory(this, realPath, requestUrlPath, loc.autoindex, loc.defaultFile);
//         }

//     Así que el flujo es:
//         Cliente pide → /carpeta/
//         ⬇
//         Servidor detecta → es un directorio
//         ⬇
//         Se llama a → Autoindex::handleDirectory(...)

//     Parámetros:
//         Client* client — puntero al objeto Client que representa la conexión actual; usado para delegar acciones como servir archivos o enviar respuestas.

//         dirPath — ruta en disco (ej. "./www/uploads") — NO la URL.

//         urlPath — la ruta URL solicitada (ej. "/uploads/") — usada en títulos y enlaces.

//         autoindexEnabled — flag (de tu configuración) que decide si mostramos listado.

//         defaultFile — nombre del archivo por defecto (p. ej. "index.html").

//     Valor devuelto: true si la operación dejó preparado _httpResponse correcto y el flujo sigue; false si hubo error (el Client esperará limpiar/close según lógica).

//     ¿Qué hace conceptualmente la función?
//     handleDirectory() recibe:
//         client → para poder enviar respuestas y porque necesitamos usar client->serveStaticFile() y client->sendError()
//         dirPath → ruta real del directorio en disco
//         urlPath → ruta tal como la pidió el navegador (p.ej. /fotos/)
//         autoindexEnabled → si está activado el autoindex en esta location
//         defaultFile → normalmente "index.html"

//     Y hace:
//     ✔️ 1) Mira si dentro del directorio existe un index.html
//         → si existe → lo sirve
//         → return

//     ✔️ 2) Si NO existe index pero autoindex está ON
//         → genera un HTML con los archivos del directorio
//         → return

//     ✔️ 3) Si NO existe index y NO hay autoindex
//         → responde 403 Forbidden

//     CÓDIGO:

//     1. Intentar servir archivo index
//         std::string indexPath = dirPath + "/" + defaultFile;

//         Construimos el path real, ejemplo:
//             dirPath: "/var/www/site/blog"
//             defaultFile: "index.html"

//             indexPath = "/var/www/site/blog/index.html"

//     2. Reserva una estructura stat para obtener info del archivo
//         if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))

//             stat() retorna 0 si el archivo existe.
//             S_ISREG() comprueba que es un archivo regular (no un directorio).

//         Llama a stat() para ver si indexPath existe y es fichero regular.
//         Si ambas son ciertas → tenemos index.html, así que delega en:
//             return client->serveStaticFile(indexPath);
//                 Qué se espera de serveStaticFile: que prepare _httpResponse con headers+body del archivo y devuelva true si todo ok.
//         Lo servimos y terminamos -> return permite que el flujo normal del Client continúe

//     3. Autoindex activado → generar listado

//         Si no hay index:

//             if (autoindexEnabled)
//                 return generateListing(client, dirPath, urlPath);

//         Esto crea una página HTML listando los archivos del directorio.

//     4. Directorio sin index y sin autoindex
//         client->sendError(403);
//         return false;

//         Si no se puede index.html y no se permite listar → devolvemos un error.
//     */

//     void generateListing(Client *client, const std::string &dirPath, const std::string &urlPath)
//     {
//         // Abrir directorio
//         DIR *dir = opendir(dirPath.c_str());
//         if (!dir)
//         {
//             // Error al abrir directorio (permisos, no existe, etc.)
//             if (errno == EACCES)
//                 client->sendError(403); // Forbidden
//             else
//                 client->sendError(404); // Not Found
//             return;
//         }

//         // Preparar HTML
//         std::ostringstream html;

//         // Escapar urlPath para HTML
//         std::string safeUrlPath = escapeHtml(urlPath);

//         // Cabecera HTML
//         html << "<!DOCTYPE html>\n"
//              << "<html>\n"
//              << "<head>\n"
//              << "  <meta charset=\"UTF-8\">\n"
//              << "  <title>Index of " << safeUrlPath << "</title>\n"
//              << "  <style>\n"
//              << "    body { font-family: Arial, sans-serif; margin: 40px; }\n"
//              << "    h1 { color: #333; margin-bottom: 20px; }\n"
//              << "    table { width: 100%; border-collapse: collapse; }\n"
//              << "    th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }\n"
//              << "    th { background-color: #f2f2f2; font-weight: bold; }\n"
//              << "    a { text-decoration: none; color: #0066cc; }\n"
//              << "    a:hover { text-decoration: underline; }\n"
//              << "    .size { text-align: right; }\n"
//              << "    .dir { font-weight: bold; }\n"
//              << "  </style>\n"
//              << "</head>\n"
//              << "<body>\n"
//              << "  <h1>Index of " << safeUrlPath << "</h1>\n"
//              << "  <table>\n"
//              << "    <tr>\n"
//              << "      <th>Name</th>\n"
//              << "      <th>Last Modified</th>\n"
//              << "      <th class=\"size\">Size</th>\n"
//              << "    </tr>\n";

//         // Enlace para subir directorio (si no estamos en raíz)
//         if (urlPath != "/" && !urlPath.empty())
//         {
//             // Encontrar directorio padre
//             std::string parentPath = urlPath;
//             size_t lastSlash = parentPath.find_last_of('/');

//             if (lastSlash == 0)
//                 parentPath = "/";
//             else if (lastSlash != std::string::npos)
//                 parentPath = parentPath.substr(0, lastSlash);

//             html << "    <tr>\n"
//                  << "      <td><a href=\"" << parentPath << "\">../</a></td>\n"
//                  << "      <td>-</td>\n"
//                  << "      <td class=\"size\">-</td>\n"
//                  << "    </tr>\n";
//         }

//         // Leer entradas del directorio
//         struct dirent *entry;
//         int entryCount = 0;
//         const int MAX_ENTRIES = 1000; // Límite por seguridad

//         while ((entry = readdir(dir)) != NULL && entryCount < MAX_ENTRIES)
//         {
//             // Saltar . y .. (ya manejamos .. manualmente)
//             std::string name = entry->d_name;
//             if (name == "." || name == "..")
//                 continue;

//             // Ruta completa del archivo
//             std::string fullPath = dirPath;
//             if (dirPath[dirPath.size() - 1] != '/')
//                 fullPath += "/";
//             fullPath += name;

//             // Obtener información del archivo
//             struct stat fileStat;
//             if (stat(fullPath.c_str(), &fileStat) != 0)
//                 continue;

//             bool isDirectory = S_ISDIR(fileStat.st_mode);

//             // Fecha
//             char dateBuf[64];
//             struct tm *timeinfo = localtime(&fileStat.st_mtime);
//             if (timeinfo)
//                 strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", timeinfo);
//             else
//                 strcpy(dateBuf, "-");
//             // TODO: REVISAR QUE SE PUEDAN USAR ESTAS FUNCIONES, O DEJARLO COMO ESTABA

//             // Tamaño
//             std::string sizeStr;
//             if (isDirectory)
//                 sizeStr = "-";
//             else
//             {
//                 // Convertir bytes a KB/MB
//                 if (fileStat.st_size < 1024)
//                 {
//                     std::ostringstream oss;
//                     oss << fileStat.st_size << " B";
//                     sizeStr = oss.str();
//                 }
//                 else if (fileStat.st_size < 1024 * 1024)
//                 {
//                     std::ostringstream oss;
//                     oss << (fileStat.st_size / 1024) << " KB";
//                     sizeStr = oss.str();
//                 }
//                 else
//                 {
//                     std::ostringstream oss;
//                     oss << (fileStat.st_size / (1024 * 1024)) << " MB";
//                     sizeStr = oss.str();
//                 }
//             }

//             // Preparar nombre para mostrar -> Añadir '/' visual a directorios
//             std::string displayName = escapeHtml(name);
//             if (isDirectory)
//                 displayName += "/";

//             // !Preparar URL
//             std::string urlEncodedName = urlEncode(name);
//             std::string itemUrl = urlPath;
//             if (itemUrl[itemUrl.size() - 1] != '/')
//             {
//                 itemUrl += "/";
//             }
//             itemUrl += urlEncodedName;

//             // !Clase CSS (directorio vs archivo)
//             std::string cssClass = isDirectory ? "dir" : "";

//             // Añadir fila a la tabla
//             html << "    <tr>\n"
//                  << "      <td class=\"" << cssClass << "\">"
//                  << "<a href=\"" << itemUrl << "\">" << displayName << "</a></td>\n"
//                  << "      <td>" << dateBuf << "</td>\n"
//                  << "      <td class=\"size\">" << sizeStr << "</td>\n"
//                  << "    </tr>\n";

//             entryCount++;
//         }

//         closedir(dir);

//         // Si llegamos al límite, mostrar advertencia
//         if (entryCount >= MAX_ENTRIES)
//         {
//             html << "    <tr>\n"
//                  << "      <td colspan=\"3\" style=\"color: #666; font-style: italic;\">"
//                  << "(Showing first " << MAX_ENTRIES << " entries)</td>\n"
//                  << "    </tr>\n";
//         }

//         // Cerrar HTML
//         html << "  </table>\n"
//              << "  <hr>\n"
//              << "  <p><em>webserv/1.0</em> - Autoindex</p>\n"
//              << "</body>\n"
//              << "</html>\n";

//         std::string htmlContent = html.str();

//         // Enviar respuesta HTML
//         client->sendHtmlResponse(htmlContent);
//         return; // TODO: REVISAR SI SE USA EL RETURN DE ESTA FUNCION EN OTROS LADOS. AQUI NO SE USA
//     }

// }

// /*
// Esta función genera una página HTML con el listado del contenido de un directorio.
// Es lo que en Apache o Nginx se llama autoindex.

//     Ejemplo:
//     Si el usuario pide:
//         GET /fotos/

//     Y no hay index.html pero la config tiene autoindex on;, tu server devuelve una página con:
//         nombre de cada archivo/subdirectorio
//         fecha de modificación
//         tamaño
//         link clicable

// generateListing() únicamente se llama:
//     ✔️ cuando el recurso es un directorio
//     ✔️ cuando no existe un archivo index
//     ✔️ cuando autoindex está activado

// CÓDIGO:

// 1. Apertura del directorio
// DIR *dir = opendir(dirPath.c_str());
// if (!dir)
// {
//     client->setErrorResponse(403);
//     return false;
// }
//     opendir() intenta abrir el directorio real del sistema (ej. /var/www/html/fotos).

//     Si falla → no podemos leer la carpeta.
//     En vez de 404 se devuelve 403 Forbidden, porque técnicamente sí existe, pero no se puede acceder.

// 2. Crear buffer HTML
// std::ostringstream html;

//     ostringstream permite construir un string largo de forma eficiente.

// 3. Cabecera HTML + CSS
//     html << "<!DOCTYPE html>\n"
//         << "<html><head>\n"
//         << "<meta charset=\"UTF-8\">\n"
//         << "<title>Index of " << urlPath << "</title>\n"
//         << "<style>"

//     Esto genera la cabecera y la metadata de la página.
//     TODO: recomendación de seguridad: escapar HTML si existe posibilidad de caracteres especiales — si urlPath proviene de la request debería escaparse para evitar XSS en nombres raros. (Más abajo doy la variante con escapeHtml.)

//     Luego se inserta CSS muy simple para hacer la tabla bonita:
//         << "body{font-family:Arial;margin:40px;}"
//         << "table{width:100%;border-collapse:collapse;}"
//         << "th,td{padding:8px;border-bottom:1px solid #ddd;}"
//         << "th{background:#f2f2f2;}"
//         << "a{text-decoration:none;color:#06c;}"
//         << "a:hover{text-decoration:underline;}"
//         << "</style>\n"
//         << "</head><body>\n";

// 4. Título y apertura de tabla
//     html << "<h1>Index of " << urlPath << "</h1>\n";
//     html << "<table><tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";

//         Crea la cabecera de la tabla que mostrará:
//             Nombre
//             Fecha modificación
//             Tamaño

// 5. Iterar por los archivos del directorio

//     struct dirent *entry;
//     while ((entry = readdir(dir)) != NULL)
//     {

//     Cada llamada a readdir() devuelve el siguiente elemento del directorio dir:
//         archivos
//         carpetas
//         . y ..
//     Si hay una entrada válida devuelve un puntero a una estructura dirent.
//     Si no hay más entradas devuelve NULL (señal de fin)

//     🔸 Saltar . y ..
//         std::string name = entry->d_name;
//         if (name == "." || name == "..")
//             continue;

//         Estos no interesa mostrarlos.
//         TODO: REVISAR: (Algunas implementaciones muestran .. como enlace al padre; puedes añadirlo explícitamente si quieres.)

//     🔸 Construir path completo para poder hacer stat()
//         std::string full = dirPath + "/" + name;

//         Ejemplo:
//         /var/www/html/fotos/imagen1.jpg

// Lo que debe aparecer en el listado del directorio es esto (que es como se ve el listado de Nginx/Apache):

//     Index of /mi_carpeta/

//     Name                Last modified          Size
//     -------------------------------------------------
//     archivo.txt         2025-01-13 10:33        821
//     imagenes/           2024-11-22 20:12         -
//     ...
//             Eso te da:
//                 🔹 Nombre del archivo/directorio
//                     Para que el usuario pueda pinchar y entrar.

//                 🔹 Última fecha de modificación (Last Modified)
//                     Es super útil:
//                         sabes cuándo se creó o editó,
//                         sabes si cambió desde la última vez,
//                         es estándar: todos los autoindex lo muestran.

//                 🔹 Tamaño
//                     Archivos: número de bytes (útil para descargas)
//                     Directorios: no tienen tamaño útil → se suele poner “-”

//                 🔹 Saber si es directorio añadiendo una barra /
//                     imagenes/ deja claro que es una carpeta.
//                     La barra final es un estándar visual y funcional (facilita rutas relativas).

//                 🔹 Estilo CSS
//                     Para que la tabla se vea bien (alineada, legible, consistente).

// Por lo tanto, seguiremos con lo siguiente:
//     🔸 Obtener información del archivo
//         struct stat st;
//         if (stat(full.c_str(), &st) != 0)
//             continue;

//         Si stat falla (link roto, permisos) saltas la entrada
//         Si no podemos obtener info del archivo → se ignora.

//     🔸 Saber si es directorio
//         bool isDir = S_ISDIR(st.st_mode);

//         Si lo es → se mostrará como "carpeta" visualmente.

//     🔸 Formatear la fecha de modificación en formato legible
//         char date[64];
//         strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S",
//                 localtime(&st.st_mtime));

//         Convierte el timestamp en formato YYYY-MM-DD HH:MM:SS.
//             TODO: REVISAR: localtime no es reentrant (usa estado interno); para tu proyecto (single-threaded webserv) está bien. Si multithread, usar localtime_r

//     🔸 Formatear el tamaño
//         std::string size = isDir ? "-" : std::to_string(st.st_size);

//         Si es carpeta → -
//         Si es archivo → tamaño en bytes.
//         TODO: Podrías mejorar a KB/MB.

//     🔸 Añadir "/" al nombre si es carpeta
//         std::string displayName = name + (isDir ? "/" : "");

//         Así queda más claro visualmente:
//             imagenes/
//             video.mp4

// 6.Insertar fila en la tabla
//     html << "<tr>"
//         << "<td><a href=\"" << urlPath
//         << (urlPath.back() == '/' ? "" : "/")
//         << name << "\">"
//         << displayName << "</a></td>"
//         << "<td>" << date << "</td>"
//         << "<td>" << size << "</td>"
//         << "</tr>\n";

//     Esto crea la fila con:
//         link correcto (urlPath respetando / final)
//         nombre
//         fecha
//         tamaño

//     TODO: IMPORTANTE: aquí no estamos haciendo URL-encoding ni HTML-escaping — en la implementación final deberías usar urlEncode(name) para la URL y escapeHtml(displayName) para el texto mostrado; lo explico abajo en la sección de seguridad.

//     TODO: urlPath.back() se usa para comprobar el último carácter; asegúrate de que urlPath no esté vacío antes de llamar back().

// 7. Cerrar directorio
// closedir(dir);

// 8. Añadir pie y cerrar HTML
// html << "</table><hr><em>webserv/1.0</em>\n";
// html << "</body></html>";

// 9. Enviar la respuesta al cliente
//     client->sendHtmlResponse(html.str());
//     return true;

//     Convierte el ostringstream en std::string y lo envía.

//     Qué debe hacer sendHtmlResponse en Client:
//         _httpResponse.setStatus(200, "OK");
//         _httpResponse.setHeader("Content-Type", "text/html");
//         _httpResponse.setHeader("Content-Length", std::to_string(htmlStr.size()));
//         _httpResponse.setBody(htmlStr);
//         applyConnectionHeader() según keep-alive.

//     Devolver true significa: respuesta lista para encolar y enviar.

// */