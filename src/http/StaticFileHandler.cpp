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
 * @brief Implementación del manejador de archivos estáticos.
 *
 * Este componente es el encargado final de interactuar con el sistema de
 * archivos. No toma decisiones de configuración (eso lo hace
 * RequestHandler), sino que ejecuta las acciones solicitadas (leer, listar,
 * subir, borrar).
 */
StaticFileHandler::StaticFileHandler() { _initMimeTypes(); }

StaticFileHandler::~StaticFileHandler() {}

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

/*
find_last_of('.') busca la última aparición de un punto en el nombre del
archivo. Ej: "index.html" → dot = 5 "archivo" → dot = npos (no hay punto)
        "application/octet-stream" es el tipo MIME genérico que se usa cuando no
sabemos el tipo de archivo (no se reconoce la extensión). En HTTP es un MIME
genérico para archivos binarios desconocidos.

fileExtension = path.substr(dot + 1) → obtiene la extensión del archivo (dot + 1
significa que hace el substring desde lo que hay justo despues del punto, hasta
el final). "index.html" → fileExtension = "html" "archivo" → no hay extensión

mimeTypes es un std::map<std::string, std::string> con los tipos MIME conocidos
*/

bool StaticFileHandler::_readFileToString(const std::string &fullPath,
                                          std::string &out, size_t size) {
  // Abrir fichero (intentar no seguir symlinks si está disponible)
  int flags = O_RDONLY;
#ifdef O_NOFOLLOW
  flags |= O_NOFOLLOW; // Suma esta flag -> Seguridad: no seguir symlinks
#endif

  int fd = open(fullPath.c_str(), flags);
  if (fd < 0) {
    // errno queda establecido por open()
    return false;
  }

  out.clear();
  try {
    out.resize(size); // Reservar memoria exacta
  } catch (const std::exception &e) {
    close(fd);
    return false;
  }

  size_t total = 0;
  while (total < size) {
    ssize_t bytesRead = read(fd, &out[total], size - total);
    if (bytesRead < 0) {
      if (errno == EINTR)
        continue; // Reintentar si fue interrumpido por una señal
      // Error real
      close(fd);
      return false;
    }
    if (bytesRead == 0) {
      // EOF inesperado (el archivo es más pequeño de lo que decía stat)
      break;
    }
    total += static_cast<size_t>(bytesRead);
  }

  close(fd);

  // Si se leyó menos de lo esperado, ajustamos el tamaño del string
  if (total < size)
    out.resize(total);

  return true;
}

// Devuelve "__FORBIDDEN__" si detecta path traversal o ruta inválida
// Devuelve "/" si path es "/".
std::string
StaticFileHandler::_sanitizePath(const std::string &decodedPath) const {
  if (decodedPath.empty())
    return "/"; // si nos piden una ruta vacía, servimos raíz

  // Debe empezar por '/'
  if (decodedPath[0] != '/')
    return "__FORBIDDEN__";

  std::vector<std::string> allParts;
  bool endsWithSlash =
      (decodedPath.size() > 1 && decodedPath[decodedPath.size() - 1] == '/');

  size_t i = 1; // saltamos la primera '/' para evitar vacío al dividir
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
      // ignorar
      continue;
    } else if (part == "..") {
      if (allParts.empty()) {
        // intento de escapar por encima del root -> prohibido
        return "__FORBIDDEN__";
      }
      allParts.pop_back();
    } else
      allParts.push_back(part);
  }

  // Reconstruir ruta limpia
  std::string cleanPath = "/";
  for (size_t k = 0; k < allParts.size(); ++k) {
    cleanPath += allParts[k];
    if (k + 1 < allParts.size())
      cleanPath += "/";
  }

  // Mantener barra final si la tenía
  if (endsWithSlash && cleanPath[cleanPath.size() - 1] != '/')
    cleanPath += "/";

  return cleanPath;
}

/**
 * @brief Procesa una petición GET para un recurso estático.
 * Resuelve la ruta real combinando root/alias y sirve el archivo o el
 * autoindex.
 */
void StaticFileHandler::handleGet(const HttpRequest &request,
                                  HttpResponse &response,
                                  const LocationConfig &location) {
  // 1. Obtener ruta (ya viene decodificada desde HttpRequest::parse)
  std::string decodedPath = request.getPath();

  // 2. Sanitizar
  std::string cleanPath = _sanitizePath(decodedPath);
  if (cleanPath == "__FORBIDDEN__") {
    response.setErrorResponse(403);
    return;
  }

  std::cout << "******************************* Path pedido:" << decodedPath
            << std::endl;

  // 4. Construir ruta final en disco (Lógica Nginx)
  // Aquí decidimos cómo mapear la URL del navegador a un archivo real en
  // nuestro disco. El administrador elige entre 'alias' o 'root' en el archivo
  // de configuración.
  std::string fullPath;
  if (location.hasAlias()) {
    // CASO ALIAS: Sustitución de prefijo.
    // Si la location (el pattern) es '/fotos' y el alias es '/data/img',
    // una petición a '/fotos/perro.jpg' se convierte en '/data/img/perro.jpg'.
    // El prefijo '/fotos' se ELIMINA y se REEMPLAZA por el alias.

    // primero eliminamos la parte del path que corresponde al pattern
    std::string relativePath = cleanPath.substr(location.getPattern().size());

    // Aseguramos que la parte relativa empiece por '/' para concatenar bien
    if (relativePath.empty() || relativePath[0] != '/')
      relativePath = "/" + relativePath;

    // Limpiamos posibles '/' al final del alias para evitar dobles '//'
    std::string aliasPath = location.getAlias();
    if (!aliasPath.empty() && aliasPath[aliasPath.size() - 1] == '/')
      aliasPath.erase(aliasPath.size() - 1);

    fullPath = aliasPath + relativePath;
    std::cout << "[DEBUG] Usando ALIAS: " << fullPath << std::endl;
  } else {
    // CASO ROOT: Anexo simple.
    // Si la location es '/fotos' y el root es '/var/www',
    // una petición a '/fotos/perro.jpg' se convierte en
    // '/var/www/fotos/perro.jpg'. El prefijo '/fotos' SE MANTIENE y se pega al
    // final del root.

    // primero obtenemos el root
    std::string rootPath = location.getRoot();

    // Limpiamos posibles '/' al final del root
    if (!rootPath.empty() && rootPath[rootPath.size() - 1] == '/')
      rootPath.erase(rootPath.size() - 1);

    fullPath = rootPath + cleanPath;
    std::cout << "[DEBUG] Usando ROOT: " << fullPath << std::endl;
  }

  std::cout << "******************************* Full Path pedido:" << fullPath
            << std::endl;

  // Comprobar existencia con stat()
  struct stat fileStat;
  if (stat(fullPath.c_str(), &fileStat) != 0) {
    // stat no pudo acceder: ENOENT → 404, EACCES → 403
    if (errno == EACCES)
      response.setErrorResponse(403);
    else
      response.setErrorResponse(404);
    return;
  }

  // 3) NUEVO: Si es directorio mostrar index o delegar en Autoindex
  if (S_ISDIR(fileStat.st_mode)) {
    std::cout << "[DEBUG] Se pide servir un directorio. Entrando en AUTOINDEX"
              << std::endl;

    _handleDirectory(fullPath, decodedPath, location, response);
    return;
  }

  // Servir archivo estático (configura la respuesta tanto de éxito como de
  // error)
  serveStaticFile(fullPath, response);
}

void StaticFileHandler::handleHead(const HttpRequest &request,
                                   HttpResponse &response,
                                   const LocationConfig &location) {
  handleGet(request, response, location); // reutiliza GET COMPLETO
  response.setBody("");                   // elimina body
}
/*
HEAD es lo mismo que get, pero no debe mostrar el body, por eso lo eliminamos
*/

void StaticFileHandler::serveStaticFile(const std::string &fullPath,
                                        HttpResponse &response) {

  // 1) Caso prohibido desde buildFullPath o sanitize
  if (fullPath == "__FORBIDDEN__") {
    response.setErrorResponse(403);
    return;
  }

  // Comprobar existencia con stat()
  struct stat fileStat;
  if (stat(fullPath.c_str(), &fileStat) != 0) {
    // stat no pudo acceder: ENOENT → 404, EACCES → 403
    if (errno == EACCES)
      response.setErrorResponse(403);
    else
      response.setErrorResponse(404);
    return;
  }

  // Protección contra archivos gigantes
  // 4) Validar tamaño
  if (fileStat.st_size < 0) {
    response.setErrorResponse(500);
    return;
  }

  size_t size = static_cast<size_t>(fileStat.st_size);
  if (size > MAX_STATIC_FILE_SIZE) {
    response.setErrorResponse(413); // Payload Too Large
    return;
  }

  // Leer archivo
  std::string content;
  if (!_readFileToString(fullPath, content, size)) {
    // open/read error → revisar errno
    if (errno == EACCES)
      response.setErrorResponse(403);
    else if (errno == ENOENT)
      response.setErrorResponse(404);
    else if (errno == EFBIG)
      response.setErrorResponse(413);
    else
      response.setErrorResponse(500);
    return;
  }

  // MIME
  std::string mime = _determineMimeType(fullPath);

  // Preparar respuesta OK
  std::ostringstream oss;
  oss << content.size();

  response.setStatus(200, "OK");
  response.setHeader("Content-Type", mime);
  response.setHeader("Content-Length", oss.str());
  response.setBody(content);

  std::cout << "[StaticFileHandler] Archivo servido: " << fullPath << "\n";
}

/*
Esta función intenta servir un fichero estático (leerlo del disco y preparar
HttpResponse con su contenido y cabeceras).

Qué es stat?
    stat es una llamada al sistema de Unix que te permite obtener información
sobre un archivo o directorio: tamaño, permisos, tipo (fichero, directorio…),
fechas, etc.

    Piensa en stat() como:
    “Oye kernel, cuéntame todo lo que sabes de este archivo.”

📌 ¿Qué devuelve exactamente stat?
La función:
    int stat(const char *path, struct stat *buf);

Rellena una estructura struct stat con datos como:

✔️ Tipo de archivo
    S_ISREG(st_mode) → fichero regular
    S_ISDIR(st_mode) → directorio
    S_ISLNK(st_mode) → enlace simbólico
    etc.

✔️ Permisos
    st_mode también contiene los permisos (rwx) del archivo.

✔️ Tamaño
    st_size → tamaño en bytes.

✔️ Fechas
    st_mtime → última modificación
    st_ctime → cambio de metadatos
    st_atime → último acceso

📁 ¿Para qué sirve en un webserver?
    Es básico para implementar:

    ✔️ 1. Comprobar si un path existe
        Si stat devuelve -1 con errno == ENOENT → 404 Not Found

    ✔️ 2. Saber si el path es un directorio
        Si es un directorio sin / final → 301 redirect
        Si es un directorio con / final → buscar índice (index.html)

    ✔️ 3. Saber si tienes permiso para leer el archivo
        Si !(st_mode & S_IROTH) → 403 Forbidden

    ✔️ 4. Saber el tamaño del archivo para enviar el header Content-Length

CÓDIGO

struct stat fileStat;
if (stat(fullPath.c_str(), &fileStat) != 0)
{
    // Manejo de error 404/403 según errno
    response.setErrorResponse(404);
    return;
}

    stat() consulta al sistema de ficheros y rellena fileStat con metadatos
(tamaño, permisos, si es directorio, timestamps, etc).

    stat(...) != 0 → stat falló (fichero no existe, permisos insuficientes, ruta
inválida) → respondemos 404 Not Found o 403 Forbidden.

    S_ISDIR(fileStat.st_mode) se comprueba en handleGet() para decidir si
servimos un archivo o entramos en Autoindex.

if (fileStat.st_size > MAX_STATIC_FILE_SIZE)
{
    response.setErrorResponse(413); // Payload Too Large
    return;
}
Antes de abrir y leer todo el archivo, nos aseguramos de que podamos soportarlo
en memoria, sino salimos.

std::string mime = _determineMimeType(fullPath);

Calcula el tipo MIME (ej. text/html, image/png) a partir de la extensión del
fullPath.

response.setStatus(200, "OK");
response.setHeader("Content-Type", mime);
// Usamos ostringstream para Content-Length (C++98)
response.setBody(content);

Construimos la respuesta con:

    200 OK

    Content-Type: <mime>

    Content-Length: <nbytes> — aquí se pone el tamaño exacto del body.
IMPORTANTE: en C++98 std::to_string no existe; usamos std::ostringstream.

    setBody(content) — coloca el contenido leído como body de la respuesta.

Conceptos nuevos que aparecen aquí (resumen)

    stat(): llamada POSIX que devuelve metadatos del fichero (tamaño, tipo,
permisos, timestamps).

    S_ISDIR(mode): macro para comprobar si mode es un directorio.

    Unix I/O (open/read/close): usamos llamadas de bajo nivel para mayor
seguridad (O_NOFOLLOW) y control de errores (EINTR).

    MIME type: tipo de contenido que indica al cliente cómo interpretar el body
(text/html, image/png...).

    Content-Length: número exacto de bytes del body; necesario si no usas
chunked.

*/

/*
ACLARACIÓN

LLAMAR A READ FILE TO STRING SIN PROTECCIÓN PREVIA
        👉 Esto funciona perfectamente para ficheros pequeños o medianos.
        ❌ Pero se convierte en un peligro serio si el cliente pide ficheros
enormes.

    Ejemplo típico:
        Te piden que sirvas un archivo de 2 GB.
        Tu servidor intenta hacer out.resize(2 * 1024 * 1024 * 1024)
        ¡Boom!
            Consumes toda tu RAM
            Matas al servidor
            El proceso es expulsado por el OOM Killer
            Cliente sin respuesta
            Server KO para todos los demás usuarios

        ➡️ Un servidor profesional nunca lee archivos grandes enteros en memoria.

    Para eso usaremos una constante que ponga un limite razonable para servir en
memoria static const size_t MAX_STATIC_FILE_SIZE = 10 * 1024 * 1024; // 10 MB

    Significa:
        Solo acepto servir archivos pequeños mediante lectura completa.
        Si el archivo es mayor, NO lo leeré entero a memoria, sino que:
            O bien respondo 413 Payload Too Large
            O bien lo sirvo por streaming, trozo a trozo (lo veremos luego)
            O devuelvo un 404 como si no existiera (menos profesional)

        10 MB es un ejemplo. Podría ser:
            1 MB → muy estricto
            50 MB → razonable
            100 MB → más generoso pero arriesgado

        En servidores reales se usa un límite para proteger el servidor.

    Por eso hacemos la protección contra archivos grandes:
        if (static_cast<size_t>(size) > MAX_STATIC_FILE_SIZE)
            return false; // o marcar error 413

    ¿Por qué es necesaria esta mejora?
        ✔️ Evita que el servidor consuma toda la RAM
            Un atacante podría llamarte con:
                GET /video_HD_9GB.mp4 HTTP/1.1

            Sin límite → tu servidor muere.

        ✔️ Evita un DDoS involuntario
            Cualquier fichero enorme en tu /www puede tumbarte.
                    DDoS = Distributed Denial of Service.
                        Muchas máquinas (o un atacante simulando muchas) te
envían peticiones diseñadas para bloquear, saturar o colapsar tu servidor, hasta
dejarlo inutilizado.

        ✔️ Servidores reales usan límites
            NGINX:
                client_max_body_size
                proxy_buffer_size
                sendfile para evitar lectura a memoria

            Apache:
                LimitRequestBody
                LimitXMLRequestBody

    De primeras piensas, “Yo no voy a tener archivos gigantes en mi disco, así
que no me afectaría, ¿no? No tengo que protegerlo” En principio sí, si tú
controlas 100% qué ficheros hay en tu carpeta www/.

        Pero…
            El evaluador puede poner cualquier archivo en tu directorio.

            En tu máquina personal o en un servidor real, cualquier usuario con
permiso podría subir un archivo enorme (upload, repositorio, backups, etc.)

            Y lo más importante:
            tu servidor no decide qué archivo existe: lo decide el sistema de
ficheros.

            Aunque tú creas que no hay archivos grandes… sí podrían aparecer.

        Ejemplo realista
            Tú crees que tu carpeta solo tiene:
                /www/index.html (3 KB)
                /www/style.css (1 KB)

            Pero puede existir fuera de tu carpeta web pero dentro de la ruta
accesible por error: /home/user/Descargas/Movie_4K_120GB.mkv

            Si por un error en tu routing construyes ese path, tu servidor
intenta leerlo → RAM muerta.

            Un atacante puede pedir cualquier ruta inventada. Si ese path
casualmente existe en el disco (por cualquier motivo): copia de un ISO, un
backup, un archivo olvidado o algo generado por otro proceso.

            Tu servidor intenta leerlo antes de decidir qué responder, por eso
la protección de tamaño es fundamental.
*/

void StaticFileHandler::_handleDirectory(const std::string &dirPath,
                                         const std::string &urlPath,
                                         const LocationConfig &location,
                                         HttpResponse &response) {
  // Extraemos la configuración de la location para este directorio
  bool autoindexEnabled = location.getAutoindex();

  // El index por defecto (ej: index.html). Si hay varios, tomamos el primero.
  std::string defaultFile =
      location.getIndex().empty() ? "" : location.getIndex()[0];

  std::cout << "[DEBUG] _handleDirectory: " << dirPath
            << ", autoindex=" << (autoindexEnabled ? "ON" : "OFF")
            << ", defaultFile=" << defaultFile << std::endl;

  // 1) PRIORIDAD 1: Intentar servir el archivo index (ej: index.html)
  // Construimos la ruta completa al archivo index dentro de la carpeta
  std::string indexPath = dirPath;
  if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
    indexPath += "/";
  indexPath += defaultFile;

  struct stat fileStat;
  // Si el archivo index existe y es un archivo regular, lo servimos y
  // terminamos. Es importante comprobar que defaultFile no esté vacío para no
  // hacer stat de la carpeta misma.
  if (!defaultFile.empty() && stat(indexPath.c_str(), &fileStat) == 0 &&
      S_ISREG(fileStat.st_mode)) {
    std::cout << "[DEBUG] Encontrado index: " << indexPath << std::endl;
    serveStaticFile(indexPath, response);
    return;
  }
  std::cout << "[DEBUG] No encontrado index o no configurado en: " << indexPath
            << std::endl;

  // 2) PRIORIDAD 2: Si no hay index, mirar si el autoindex está activado. Si es
  // así, generamos el listado.
  if (autoindexEnabled) {
    std::cout << "[DEBUG] Autoindex ON → Generando listado HTML para: "
              << dirPath << std::endl;
    std::string html = Autoindex::generateListing(dirPath, urlPath);
    if (html.empty()) {
      // Si opendir falló dentro de generateListing
      if (errno == EACCES)
        response.setErrorResponse(403);
      else
        response.setErrorResponse(404);
      return;
    }
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/html");
    response.setBody(html);
    return;
  }

  // 3) SI NADA DE LO ANTERIOR FUNCIONA: Error 403 (Prohibido listar)
  // Esto es lo que hace Nginx por defecto si no hay index y el autoindex está
  // OFF.
  std::cout << "[DEBUG] Sin index y autoindex OFF → 403 Forbidden" << std::endl;
  response.setErrorResponse(403);
}

/*
Su propósito es: Decidir qué hacer cuando la ruta solicitada por el cliente
apunta a un directorio.

Cuando el servidor recibe una petición GET, tú determinas si el path real del
sistema es: un archivo ✔️ → se sirve un directorio 📁 → debes decidir qué hacer
    inexistente ❌ → error

Cuando es un directorio, tienes 3 opciones:
    Servir un archivo index.html (si existe)
    Mostrar un listado de ficheros (autoindex on)
    Devolver 403 Forbidden (si no hay index y autoindex está off)

_handleDirectory() implementa justamente este flujo.

Por lo tanto, esta función decide si servir un defaultFile (p. ej. index.html) o
generar el listado si autoindexEnabled == true. En caso contrario, envía error
403.

La llama tu manejador principal de rutas en el servidor
Cuando haces:
    struct stat fileStat;
    stat(realPath.c_str(), &fileStat);

    if (S_ISDIR(fileStat.st_mode))
    {
        return _handleDirectory(realPath, requestUrlPath, location, response);
    }

Así que el flujo es:
    Cliente pide → /carpeta/
    ⬇
    Servidor detecta → es un directorio
    ⬇
    Se llama a → _handleDirectory(...)

Parámetros:
    dirPath — ruta en disco (ej. "./www/uploads") — NO la URL.

    urlPath — la ruta URL solicitada (ej. "/uploads/") — usada en títulos y
enlaces.

    location — configuración de la ubicación actual.

    response — objeto respuesta donde se cargará el resultado.

¿Qué hace conceptualmente la función?
_handleDirectory() recibe:
    dirPath → ruta real del directorio en disco
    urlPath → ruta tal como la pidió el navegador (p.ej. /fotos/)
    location → configuración que incluye autoindex e index files

Y hace:
✔️ 1) Mira si dentro del directorio existe un index.html
    → si existe → lo sirve
    → return

✔️ 2) Si NO existe index pero autoindex está ON
    → genera un HTML con los archivos del directorio
    → return

✔️ 3) Si NO existe index y NO hay autoindex
    → responde 403 Forbidden

CÓDIGO:

1. Intentar servir archivo index
    std::string indexPath = dirPath + "/" + defaultFile;

Construimos el path real, ejemplo:
    dirPath: "/var/www/site/blog"
    defaultFile: "index.html"

    indexPath = "/var/www/site/blog/index.html"

2. Reserva una estructura stat para obtener info del archivo
    if (stat(indexPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode))

        stat() retorna 0 si el archivo existe.
        S_ISREG() comprueba que es un archivo regular (no un directorio).

    Llama a stat() para ver si indexPath existe y es fichero regular.
    Si ambas son ciertas → tenemos index.html, así que delega en:
        serveStaticFile(indexPath, response);

3. Autoindex activado → generar listado

    Si no hay index:

        if (autoindexEnabled)
            std::string html = Autoindex::generateListing(dirPath, urlPath);

    Esto crea una página HTML listando los archivos del directorio.

4. Directorio sin index y sin autoindex
    response.setErrorResponse(403);

    Si no se puede index.html y no se permite listar → devolvemos un error.
*/

/**
 * @brief Maneja las peticiones POST (principalmente para subida de
 * archivos).
 *
 * El flujo de esta función es:
 * 1. Validar que no sea chunked (no soportado actualmente).
 * 2. Obtener y validar el directorio de subida (upload_path).
 * 3. Crear el directorio si no existe.
 * 4. Generar un nombre de archivo único para evitar colisiones.
 * 5. Escribir el cuerpo de la petición en el archivo de forma segura.
 * 6. Responder con 201 Created.
 */
void StaticFileHandler::handlePost(const HttpRequest &request,
                                   HttpResponse &response,
                                   const LocationConfig &location) {

  // 1: Validar Transfer-Encoding
  // Actualmente no soportamos subidas 'chunked'. Si el cliente lo intenta,
  // respondemos con 501 Not Implemented.
  if (request.isChunked()) {
    response.setStatus(501, "Not Implemented");
    response.setHeader("Content-Type", "text/html");
    response.setBody("<html><body><h1>501 Not Implemented</h1>"
                     "<p>Chunked uploads are not supported.</p>"
                     "</body></html>");
    return;
  }

  // 2: Obtener directorio de destino desde location
  // El directorio viene definido por la directiva 'upload_path' en la
  // configuración.
  std::string uploadDir = location.getUploadPath();
  if (uploadDir.empty()) {
    // Si no hay ruta de subida configurada, es un error del servidor.
    response.setErrorResponse(500);
    return;
  }

  // 3: Verificar existencia del directorio y crearlo si no existe
  struct stat fileStat;
  if (stat(uploadDir.c_str(), &fileStat) != 0) {
    // Si el directorio no existe (ENOENT), intentamos crearlo.
    if (errno == ENOENT) {
      if (mkdir(uploadDir.c_str(), 0755) !=
          0) // Permisos 0755 → lectura + escritura para owner, lectura para
             // otros. Si falla, damos error del servidor
      {
        response.setErrorResponse(500); // Error al crear carpeta
        return;
      }
    } else // stat falló por otra razón → error
    {
      response.setErrorResponse(500); // Error de sistema (permisos, etc.)
      return;
    }
    // MANDAMOS
    // EL MISMO CODIGO?
  } else if (!S_ISDIR(fileStat.st_mode)) {
    // Si existe pero no es un directorio (es un archivo), error.
    response.setErrorResponse(500);
    return;
  }

  // Comprobar que tenemos permisos de escritura en la carpeta
  if (access(uploadDir.c_str(), W_OK) != 0) {
    response.setErrorResponse(403); // Prohibido escribir aquí
    return;
  }

  // 4: Generar nombre de archivo único
  // Usamos el tiempo actual, el PID del proceso y un número aleatorio
  // para minimizar la probabilidad de que dos subidas coincidan en nombre.
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

  // 5: Escribir el archivo en disco
  // O_CREAT | O_EXCL garantiza que si por un milagro el archivo ya existiera,
  // la apertura fallaría en lugar de sobrescribirlo.
  int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd == -1) {
    response.setErrorResponse(500);
    return;
  }

  // Escribimos el cuerpo (body) de la petición en el archivo
  const std::string &body = request.getBody();
  const char *buf = body.data(); // aunque body ya tiene los datos, no podemos
                                 // usarlos directamente con write, porque
                                 // trabaja con punteros a bytes
  size_t buf_size = body.size();
  size_t written = 0; // para saber cuánto llevamos escrito

  // Bucle de escritura para asegurar que se envían todos los bytes.
  // write()puede escribir menos bytes de los pedidos, por eso hacemos bucle
  while (written < buf_size) {
    ssize_t ret = write(fd, buf + written, buf_size - written);
    if (ret < 0) {
      if (errno == EINTR)
        continue; // Si una señal interrumpe write → repetir.

      // Error grave: cerramos y borramos el archivo incompleto (limpieza)
      close(fd);
      unlink(filepath.c_str()); // Limpiamos el archivo corrupto con unlink()
      response.setErrorResponse(500);
      return;
    }
    written += static_cast<size_t>(ret);
  }

  fsync(fd); // Fuerza la escritura del archivo físicamente a disco antes de
             // cerrar.
  close(fd);

  // 6: Responder al cliente - Preparar respuesta HTTP
  // Según el estándar, una subida exitosa debe devolver 201 Created.
  response.setStatus(201, "Created");
  response.setHeader("Content-Type", "text/html");
  response.setHeader("Location", "/uploads/" + filename); // Dónde encontrarlo

  std::ostringstream html;
  html << "<html><body>"
       << "<h1>Upload successful</h1>"
       << "<p>Saved as: " << filename << " (" << body.size() << " bytes)</p>"
       << "</body></html>";

  response.setBody(html.str());

  std::cout << "[POST] Upload OK => " << filename << " (" << body.size()
            << " bytes)" << std::endl;
}

/**
 * @brief Maneja las peticiones DELETE.
 *
 * El flujo de esta función es:
 * 1. Resolver la ruta física del archivo (usando root o alias).
 * 2. Verificar que el archivo existe y no es un directorio.
 * 3. Verificar permisos de escritura en el directorio padre.
 * 4. Intentar borrar el archivo con std::remove.
 * 5. Responder con 204 No Content (éxito) o el error correspondiente.
 */
void StaticFileHandler::handleDelete(const HttpRequest &request,
                                     HttpResponse &response,
                                     const LocationConfig &location) {
  // 1. Obtener ruta (ya viene decodificada desde HttpRequest::parse)
  std::string decodedPath = request.getPath();

  // 2. Sanitizar
  std::string cleanPath = _sanitizePath(decodedPath);

  if (cleanPath == "__FORBIDDEN__") {
    response.setErrorResponse(403);
    return;
  }

  // Construimos la ruta final en disco siguiendo la misma lógica que en GET
  std::string fullPath;
  if (location.hasAlias()) {
    // Caso ALIAS: Sustitución de prefijo
    std::string relativePath = cleanPath.substr(location.getPattern().size());
    if (relativePath.empty() || relativePath[0] != '/')
      relativePath = "/" + relativePath;

    std::string aliasPath = location.getAlias();
    if (!aliasPath.empty() && aliasPath[aliasPath.size() - 1] == '/')
      aliasPath.erase(aliasPath.size() - 1);

    fullPath = aliasPath + relativePath;
  } else {
    // Caso ROOT: Anexo simple
    std::string rootPath = location.getRoot();
    if (!rootPath.empty() && rootPath[rootPath.size() - 1] == '/')
      rootPath.erase(rootPath.size() - 1);

    fullPath = rootPath + cleanPath;
  }

  std::cout << "[DEBUG] DELETE fullPath: " << fullPath << std::endl;

  // 2: Verificar que el archivo existe y que no es un directorio
  struct stat fileStat;
  if (stat(fullPath.c_str(), &fileStat) != 0) {
    // Si stat falla, el archivo no existe o no tenemos permiso para verlo
    if (errno == ENOENT)
      response.setErrorResponse(404); // No encontrado, el archivo no existe
    else if (errno == EACCES)
      response.setErrorResponse(403); // Prohibido
    else
      response.setErrorResponse(500); // Error interno
    return;
  }

  // Por seguridad, no permitimos borrar directorios a través de DELETE
  if (S_ISDIR(fileStat.st_mode)) {
    response.setErrorResponse(403);
    return;
  }

  // 3: Verificar permisos de borrado
  // Para borrar un archivo, necesitamos permisos de escritura en la CARPETA que
  // lo contiene.
  std::string parentDir = fullPath.substr(0, fullPath.find_last_of('/'));
  if (parentDir.empty())
    parentDir = ".";

  if (access(parentDir.c_str(), W_OK) != 0) {
    response.setErrorResponse(403); // No tenemos permiso para borrar
    return;
  }

  // --- PASO 4: Borrar el archivo ---
  if (std::remove(fullPath.c_str()) != 0) {
    if (errno == EACCES || errno == EPERM) {
      response.setErrorResponse(403);
    } else {
      response.setErrorResponse(500);
    }
    return;
  }

  // --- PASO 5: Responder al cliente ---
  // El estándar HTTP (RFC) recomienda 204 No Content para borrados exitosos
  response.setStatus(204, "No Content");
  response.setBody("");

  std::cout << "[DELETE] File removed OK => " << fullPath << std::endl;
}
