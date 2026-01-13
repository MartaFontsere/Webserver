#include "http/HttpRequest.hpp"
#include <cstdlib> // para atoi
#include <cstring> // para strcasecmp, memset, etc.
#include <iostream>
#include <sstream>
#include <strings.h> // para strcasecmp

/*
El límite de tamaño de body ahora viene del config (client_max_body_size)
y se verifica en RequestHandler, no aquí. Esto permite que cada location
tenga su propio límite configurado.
*/

HttpRequest::HttpRequest()
    : _headersComplete(false), _isChunked(false), _keepAlive(false),
      _isMalformed(false), _parsedBytes(0), _contentLength(-1) {}

/**
 * @brief Función principal de parseo. Es "progresiva" y se llama cada vez que
 * recibimos datos.
 *
 * @param rawRequest El string con los datos brutos acumulados del socket.
 * @return true si la petición está COMPLETA (headers + body si lo hay).
 * @return false si aún faltan datos por recibir.
 */
bool HttpRequest::parse(const std::string &rawRequest) {
  // Reiniciamos el contador de bytes consumidos en esta llamada (por si se ha
  // llamado varias veces con el mismo cliente)
  _parsedBytes = 0;

  // --- ETAPA 1: Parsear Cabeceras ---
  // Si aún no hemos terminado de procesar los headers, seguimos intentando
  // leerlos todos.
  if (!_headersComplete) {
    if (!parseHeaders(rawRequest))
      return false; // Aún no ha llegado el doble salto de línea (\r\n\r\n), por
                    // lo que no se han leido todos los headers.
  }

  // --- ETAPA 2: Parsear Cuerpo (Body) ---
  // Si los headers están malformados, no seguimos con el cuerpo.
  if (_isMalformed)
    return true;

  // Una vez tenemos los headers, comprobamos si la petición requiere parsear un
  // cuerpo. Esto ocurre si hay 'Content-Length' > 0 o si es 'Transfer-Encoding:
  // chunked'.
  if (_headersComplete && (_contentLength > 0 || _isChunked)) {
    if (!parseBody(rawRequest))
      return false; // Aún no tenemos todos los bytes del cuerpo esperados.
  }

  // ✅ ÉXITO: Si llegamos aquí, la petición está completa y lista para ser
  // procesada.
  return true;
}

bool HttpRequest::isMalformed() const { return _isMalformed; }

/*
18.11.25
HttpRequest::parse(raw) seguirá devolviendo true/false, pero cuando devuelve
true dejará almacenado cuántos bytes ha consumido (cabeceras + body). Añadimos
int _parsedBytes y size_t getParsedBytes() const.

En Client::readRequest() no borramos todo _rawRequest al parsear: llamaremos
_rawRequest.erase(0, _httpRequest.getParsedBytes()). Así si vienen bytes extra
(pipelined requests) se quedan listos.
*/

/*
HttpRequest
    Se encarga de interpretar los datos recibidos: método, path, versión,
headers, body.

    Lleva sus propios flags: _headersComplete, _bodyComplete.

    Devuelve true o false desde parse() según si ya tiene todo lo necesario.

    👉 Así que HttpRequest devuelve el estado, y Client lo usa para marcar su
_requestComplete.

Queremos que HttpRequest::parse() haga esto, de forma progresiva:
    Ver si ya tenemos el final de los headers → si no, seguimos leyendo.

    Parsear los headers → guardar _method, _path, _version y los pares
clave-valor.

    Detectar si hay body (por Content-Length o Transfer-Encoding: chunked).

    Si lo hay, esperar a tenerlo entero antes de continuar.
*/

bool HttpRequest::parseHeaders(const std::string &rawRequest) {
  // 🔍 Comprobamos si la petición HTTP está completa
  // Buscamos el final de la cabecera (header) HTTP, que termina con "\r\n\r\n"
  size_t headerEnd = rawRequest.find("\r\n\r\n");
  if (headerEnd ==
      std::string::npos) // significa “no encontrado” o “posición inválida”
    return false;        // aún no ha llegado toda la cabecera

  _headersComplete =
      true; // Si llega hasta aquí, significa que ha encontrado
            // el final, ha recibido todas las cabeceras (headers, no el body)
  _parsedBytes = headerEnd + 4; // Headers + \r\n\r\n
  // A partir de ahora, se puede intentar parsear (interpretar) lo que se ha
  // recibido.

  // Extraemos y guardamos solo la parte de la cabecera
  std::string headerPart = rawRequest.substr(0, headerEnd);

  std::istringstream ss(headerPart);
  std::string line;

  // Primera línea → siempre tiene esta forma: método, path, versión
  if (!std::getline(ss, line))
    return false;

  std::istringstream firstLine(line);
  std::string fullTarget;
  std::string extra;
  if (!(firstLine >> _method >> fullTarget >> _version) ||
      (firstLine >> extra)) {
    std::cout << "[Debug] Malformed request line: " << line << std::endl;
    _isMalformed = true;
    return true; // Terminamos el parseo pero marcamos error
  }

  // Separar PATH y QUERY
  size_t qpos = fullTarget.find('?');
  if (qpos != std::string::npos) {
    _path = _urlDecode(fullTarget.substr(0, qpos), false);
    _query = _urlDecode(fullTarget.substr(qpos + 1), true);
  } else {
    _path = _urlDecode(fullTarget, false);
    _query.clear();
  }

  if (_version == "HTTP/1.1")
    _keepAlive = true; // por defecto en HTTP/1.1
  else
    _keepAlive = false; // por defecto en HTTP/1.0

  // Resto de líneas → headers
  while (std::getline(ss, line)) {
    if (line == "\r" || line.empty())
      break;

    size_t pos = line.find(":");
    if (pos == std::string::npos)
      continue;

    std::string key = line.substr(0, pos);
    std::string val = line.substr(pos + 1);

    // Limpieza de espacios y '\r'
    if (!val.empty() && val[0] == ' ')
      val.erase(0, 1);
    if (!val.empty() && val[val.length() - 1] == '\r')
      val.erase(val.length() - 1); // Eliminar '\r' al final

    // Normalizar key a lowercase para búsqueda case-insensitive
    for (size_t i = 0; i < key.length(); ++i) {
      if (key[i] >= 'A' && key[i] <= 'Z')
        key[i] = key[i] - 'A' + 'a';
    }

    _headers[key] = val;

    // Detectar Content-Length y Transfer-Encoding
    if (strcasecmp(key.c_str(), "content-length") == 0)
      _contentLength = atoi(val.c_str());
    else if (strcasecmp(key.c_str(), "transfer-encoding") == 0 &&
             val.find("chunked") != std::string::npos)
      _isChunked = true;

    // Aunque ya está puesto por defecto, aqui permites sobreescrivir si se
    // especifica lo contrario en connection
    if (strcasecmp(key.c_str(), "connection") == 0) {
      if (strcasecmp(val.c_str(), "close") == 0)
        _keepAlive = false;
      else if (strcasecmp(val.c_str(), "keep-alive") == 0)
        _keepAlive = true;
    }
    // No es redundante, primero parseasmos la versión y defines el default,
    // luego parseamos los headers y defines si el cliente quiere cambiar ese
    // default. Así es como funciona el protocolo
  }

  // VALIDACIÓN: Header Host es obligatorio en HTTP/1.1
  // Buscamos "host" en minúsculas porque acabamos de normalizar
  if (_version == "HTTP/1.1" && _headers.find("host") == _headers.end()) {
    std::cout << "[Debug] HTTP/1.1 request missing Host header" << std::endl;
    _isMalformed = true;
  }

  return true;
}

static int hexVal(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  return -1;
}

/*
¿Por qué existen urlEncode y urlDecode?
    Cuando un navegador envía una URL, no puede enviar caracteres especiales tal
cual, siempre envía el path codificado.

    Esto NO es válido en una URL:
        /file with spaces.txt

    El navegador lo convierte automáticamente en:
        /file%20with%20spaces.txt

    Esto pasa siempre, independientemente de que escribas la URL a mano, hagas
clic, vengas de autoindex... al servidor siempre le llega codificado. Por eso
hay que decodificar

    Ejemplos de caracteres problemáticos:
        espacio
        á é í ó ü
        # (marca fragmentos)
        ? (abre query string)
        / (separador)
        % (inicio de codificación)
        : (protocolo)
        ;
        "

    Si los enviara tal cual, rompería la sintaxis del protocolo.


Solución del estándar: URL encoding (RFC 3986)
    La URL debe codificar esos caracteres raros como:
        %XX   ← valor hexadecimal del byte

    Ejemplos:
        "hola mundo" → hola%20mundo
        ñ           → %C3%B1  (UTF-8)
        ?           → %3F
        #           → %23


    Esto significa que cuando el servidor recibe una URL, NO es la URL real:
    es una versión escapada → tu servidor debe decodificarla para trabajar con
rutas reales del sistema de archivos.

¿POR QUÉ ES IMPORTANTE PARA WEBSERV?
    Porque sin esto:

        /hola%20marta.txt → buscarías un archivo literal con %20 en el nombre
        (y fallaría con 404)

        consulta GET con parámetros ?name=Marta+Fontseré
        → recibirías Marta+Fontseré en vez de Marta Fontseré

        autoindex mostrando rutas tendría enlaces rotos

        seguridad: ataques de path traversal pueden venir codificados:
            ..%2F..%2Fetc/passwd


    Por eso es OBLIGATORIO para cualquier servidor web serio.

¿Y si el archivo se llama literalmente file%20.txt?
    Archivo real:
        file%20.txt

    Para pedirlo correctamente:
        El % debe codificarse como %25

            /file%2520.txt

        Decodificación:
             %25 → %

        Resultado final:
            file%20.txt

Qué es una query string?
    Una query string es la parte opcional de la URL que va después del ?

    Ejemplo:
        /search?q=hello+world&page=2
               ↑
               query string

    La URL se divide así:
        /search          → PATH
        ?q=hello+world   → QUERY STRING

    👉 NO son lo mismo
    👉 Se procesan distinto
    👉 Se codifican distinto

PATH vs QUERY

| Parte     | Qué es                | Para qué se usa           |
| --------- | --------------------- | ------------------------- |
|   PATH    | Identifica el recurso | Archivo / directorio      |
|   QUERY   | Parámetros            | Búsquedas, filtros, flags |

Ejemplo:
    /images/my photo.jpg?size=large

    PATH → /images/my photo.jpg
    QUERY → size=large

    El archivo es el mismo, cambie lo que cambie la query

¿Puede llegar una query string al webserver?
    Sí, totalmente.
    Cualquier request HTTP puede traerla:
        GET /file.txt?download=true HTTP/1.1

    Tu parser HTTP debería separar:
        path → /file.txt
        query → download=true

    Importante:
        La query NO forma parte del path del archivo.
        El filesystem no debe verla.


¿Y el carácter +? (esta es la trampa)
    + NO significa espacio en el PATH

    En URLs:
        PATH → espacios = %20
        QUERY → espacios = + (solo en form encoding)


Reglas definitivas para tu webserver (guárdalas)
    ✔ PATH
        Siempre viene URL-encoded
        Espacios → %20
        + es literal
        Decodifica %XX
        NO conviertas + → space
        Decodifica antes de sanitizePath

    ✔ QUERY STRING
        Espacios pueden venir como +
        Decodifica %XX
        Convierte + → space

    ✔ Autoindex
        Genera URLs codificadas (urlEncode)
        Usa %20 para espacios


CÓDIGO:
    Objetivo:
        Tomar una cadena así:
            /hola%20marta/archivo%2Etxt

        y convertirla en:
            /hola marta/archivo.txt

std::string decoded;
    Se crea la cadena que devolveremos, donde iremos añadiendo los caracteres ya
decodificados.

decoded.reserve(encoded.size());
    Reservamos capacidad para decoded igual al tamaño de la cadena de entrada.
    Por qué: evita realocaciones internas al push_back/operator+= y mejora
rendimiento. Nota: el tamaño final nunca será mayor que encoded.size() (de hecho
suele ser ≤), así que es una reserva razonable.

for (size_t i = 0; i < encoded.size(); ++i)
    Recorre cada carácter de la cadena

    🔸 Caso 1 — detecta %XX (el inicio de una secuencia percent-encoded)
        if (encoded[i] == '%' && i + 2 < encoded.size())

        Esto significa:
            encoded[i] == '%' → el carácter % indica codificación, por lo que
indica que viene una secuencia %XX i + 2 < encoded.size() → faseguramos que hay
al menos dos caracteres hex detrás (% + 2 hex) para no salirnos del buffer
        Importante: si hay un % al final sin dos hex, este if será falso y se
tratará más abajo como carácter normal

        int high = hexVal(encoded[i + 1]);
        int low = hexVal(encoded[i + 2]);
            Usamos la función auxiliar hexVal para convertir los dos caracteres
hexadecimales a sus valores numéricos correspondientes.

        if (high >= 0 && low >= 0)
            Si ambos caracteres eran hexadecimales válidos, calculamos el byte
final combinando ambos nibbles: (high << 4) | low.

        decoded.push_back(static_cast<char>((high << 4) | low));
            Añadimos el carácter resultante a la cadena decodificada.

        i += 2; // saltamos los dos hexadecimales que acabamos de procesar.

    🔸 Caso 2 — detecta +
        else if (c == '+' && plusAsSpace)
            Si estamos decodificando una QUERY STRING (plusAsSpace = true),
convertimos el + en un espacio. Si es el PATH, lo dejamos como un + literal.

    🔸 Caso 3 — cualquier otro carácter
        decoded.push_back(c);
            Si no requiere decode, lo dejamos igual.

Devolvemos la cadena decodificada.
*/
std::string HttpRequest::_urlDecode(const std::string &encoded,
                                    bool plusAsSpace) const {
  std::string decoded;
  decoded.reserve(encoded.size()); // Reservar memoria para evitar realocaciones

  for (size_t i = 0; i < encoded.size();
       ++i) // Recorrer cáda caracter de la cadena
  {
    char c = encoded[i];
    if (c == '%' && i + 2 < encoded.size()) {
      int highNibble =
          hexVal(encoded[i + 1]); // guardamos el valor después de %
      int lowNibble =
          hexVal(encoded[i + 2]); // guardamos el valor dos veces después de %
      if (highNibble >= 0 && lowNibble >= 0) {
        decoded.push_back(static_cast<char>(
            (highNibble << 4) |
            lowNibble)); // pusheamos los dos valores convertidos a hexadecimal
                         // haciendo movimiento de bits, para reconstruir el
                         // byte y decodificarlo (pasar de %2B, a high Nibble 2
                         // y lowNibble 11, en hexadecimal, y al juntarlo en
                         // bits sea 2 → 0010 y 11 → 1011, y por lo tanto 0010
                         // 1011 = 0x2B = '+', decodificado)
        i += 2; // saltamos los dos hex procesados
      } else {
        // secuencia mal formada: conservador → dejamos '%' literal
        decoded.push_back('%');
        // no saltamos, así G y Z se procesarán en siguientes iteraciones
      }
    } else if (c == '+' && plusAsSpace) {
      // Solo convertir '+' → ' ' si explícitamente pedimos plusAsSpace=true.
      decoded.push_back(' ');
    } else {
      decoded.push_back(c);
    }
  }
  return decoded;
}

/*
Explicación clave:
    _headersComplete = true → marca que ya tenemos todos los headers.
    _isChunked servirá luego para procesar bodies en modo chunked encoding.
    Esta función no toca aún el body, solo se encarga de analizar los headers.

1.
Comprobación de si la petición está completa
    En HTTP, las cabeceras siempre terminan con \r\n\r\n.
    Así que si aún no se ha recibido ese separador, significa que todavía no
tenemos toda la cabecera (quizá llegó a medias), por eso devolvemos false para
que el servidor siga esperando más datos.

2.
Marcar que ya se han recibido todas las cabeceras
    Aquí decimos “ok, ya tengo la parte de las cabeceras completa”.
    A partir de ahora, se puede intentar parsear (interpretar) lo que se ha
recibido.

3.
Extraer solo la parte de las cabeceras (sin el body)
    _request contiene todo lo que ha llegado del socket hasta el momento (quizá
también parte del body).

    Pero las cabeceras están solo hasta el \r\n\r\n.

    Así que extraemos solo esa parte.

4.
Leer línea por línea

std::istringstream ss(headerPart);
std::string line;

    Creamos un flujo de texto (ss) para poder leer línea por línea, igual que
leer un archivo con getline().

    std::istringstream es una “entrada de texto” desde un std::string, igual que
std::cin es una entrada desde el teclado. 👉 En lugar de leer caracteres que
escribe el usuario, lees caracteres desde un string que ya tienes. Ejemplo:
        std::string data = "123 456 789";
        std::istringstream ss(data);

        int a, b, c;
        ss >> a >> b >> c;
    Esto “extrae” los números del string como si vinieran por cin.
    Al final:
        a = 123
        b = 456
        c = 789

    Porque convertir a istringstream????
        Un std::string es solo una cadena de caracteres, sin significado
especial. Ejemplo: std::string text = "Hola\nMundo\nBonito";

            Esto guarda exactamente los caracteres:
                H o l a \n M u n d o \n B o n i t o

            Es decir, el carácter \n está ahí, pero el string no lo convierte en
salto de línea por sí mismo; simplemente lo contiene.

        std::istringstream permite leer el string como si fuera texto “en
streaming” Cuando haces: std::istringstream ss(text); le estás diciendo: “Quiero
tratar el contenido del string text como si fuera una entrada de texto (como
std::cin) que puedo leer línea a línea o palabra a palabra”.

            No cambia nada en el contenido: simplemente te da una forma de
recorrerlo.

    Usar getline() con un istringstream
        getline() se usa para leer líneas completas (hasta el carácter de salto
de línea \n) Lo que hace internamente es: Empieza a leer el flujo (ss) carácter
a carácter

            Copia esos caracteres en line

            Se detiene justo cuando encuentra un \n (salto de línea)

            El \n no se incluye en line

        🔸 Devuelve false cuando ya no quedan líneas

    por qué no usar directamente el string?
        Porque un std::string no tiene posición de lectura.
        No puedes “ir leyendo” línea a línea sin cortar y copiar manualmente con
find("\n"), substr(), etc.

        El std::istringstream te evita eso:
            Mantiene una posición interna

            Te da operaciones cómodas (>>, getline, etc.)

            Funciona igual que std::cin o std::ifstream

Resumen:
| Elemento                 | Qué hace | | ------------------------ |
------------------------------------------------------- | | `std::string` | Solo
guarda texto tal cual                              | | `std::istringstream` | Te
permite leer ese texto **como si fuera una entrada** | | `getline(ss, line)` |
Lee hasta `\n` y guarda una línea                       | | `>>` con
`istringstream` | Extrae “palabras” separadas por espacios o tabs         |

5.
Primera línea → método, path y versión
        VERSIÓN SIMPLE:
                if (!std::getline(ss, line))
                    return false;
                {
                    std::istringstream firstLine(line);
                    firstLine >> _method >> _path >> _version;
                }

                    La primera línea de toda petición HTTP tiene esta forma:
                    GET /index.html HTTP/1.1

                    Por tanto:
                        _method = GET
                        _path = /index.html
                        _version = HTTP/1.1

        std::getline(ss, line);
        lee la primera línea completa (por ejemplo: "GET /index.html HTTP/1.1")

        Si no hay primera línea, devuelve false

        Ojo: los { } después del if no pertenecen al if.
        Son un bloque independiente que se ejecuta siempre, después del if.
            Se crea un bloque nuevo para limitar el alcance de variables
locales.

            Dentro, se crea un istringstream llamado firstLine que contiene esa
línea.

            Luego se extraen tres tokens separados por espacios: el método
(GET), la ruta (/index.html) y la versión (HTTP/1.1).

            Al acabar el bloque se borra esa variable firstLine. Las llaves {}
crean un bloque local temporal para que variables como firstLine existan solo
ahí dentro


        ahora quiero separar los tres elementos de esa línea:
        std::istringstream firstLine(line);
        firstLine >> _method >> _path >> _version;

        Lo que ocurre es:
            ss sirve para recorrer todo el bloque de texto línea a línea
            getline(ss, line) obtiene la primera línea
            firstLine es un nuevo istringstream que lee esa línea palabra a
palabra

        👉 Así consigues dividir



        VERSIÓN COMPLETA:
            {
                std::istringstream firstLine(line);
                std::string fullTarget;
                firstLine >> _method >> fullTarget >> _version;

                size_t qpos = fullTarget.find('?');
                if (qpos != std::string::npos)
                {
                    _path  = fullTarget.substr(0, qpos);
                    _query = fullTarget.substr(qpos + 1);
                }
                else
                {
                    _path = fullTarget;
                    _query.clear();
                }
            }

        Esta parte parsea la primera línea de una petición HTTP, que siempre
tiene esta forma: <METHOD> <TARGET> <VERSION>

        Ejemplos reales:
            GET /index.html HTTP/1.1
            GET /tests/files/?sort=name HTTP/1.1
            POST /upload?user=marta HTTP/1.0

        Tu objetivo es extraer:
            _method → "GET"
            _path → "/tests/files/"
            _query → "sort=name"
            _version → "HTTP/1.1"

        👉 Esa línea llega como un string completo, por ejemplo:
        line = "GET /tests/files/?sort=name HTTP/1.1";

        std::istringstream es un stream de entrada, pero en vez de leer de
teclado o de archivo, lee de un string. Es como decir: “Voy a tratar este string
como si fuera un flujo de texto del que puedo extraer palabras”. El stream queda
así internamente: GET | /index.html | HTTP/1.1


        Aquí declaras una variable temporal -> std::string fullTarget;

        ¿Por qué no escribir directamente _path aquí?
        Porque el target HTTP puede contener query string, no solo path. Así que
primero lo guardas completo y luego lo separas.

        firstLine >> _method >> fullTarget >> _version;
            Esta línea es CLAVE.
                El operador >> en streams:
                Lee hasta el próximo espacio
                Ignora espacios múltiples
                Funciona como “sacar palabras”

            Entonces esto hace:
            | Variable     | Valor                       |
            | ------------ | --------------------------- |
            | `_method`    | `"GET"`                     |
            | `fullTarget` | `"/tests/files/?sort=name"` |
            | `_version`   | `"HTTP/1.1"`                |

            Si la línea fuera inválida (faltan cosas), el stream fallaría
            (algo que luego puedes validar)

        Luego separamos path y query string.
            Ahora tenemos:
                fullTarget = "/tests/files/?sort=name";

            Pero queremos:
                _path = "/tests/files/"
                _query = "sort=name"

        size_t qpos = fullTarget.find('?');
            Esto busca el carácter ? dentro del string.

            find():
                Devuelve la posición del carácter
                Si no existe, devuelve std::string::npos

        if (qpos != std::string::npos)
            Si existe query string, hay ?, entramos y dividimos de inicio hasta
interrogante y de interrogante en adelante. El interrogante no lo incluimos,
solo los parámetros

        else
            Si no hay query, el path es todo
            La query se vacía (muy importante para no arrastrar datos de
peticiones anteriores)


6.
while (std::getline(ss, line))
{
    if (line == "\r" || line.empty())
        break;
Aquí se procesan las siguientes líneas, hasta llegar a una línea vacía (\r\n),
que marca el final de las cabeceras.

7. Separar clave y valor:
size_t pos = line.find(":");
if (pos == std::string::npos)
    continue;

std::string key = line.substr(0, pos);
std::string val = line.substr(pos + 1);

8.
Limpiar espacios y caracteres sobrantes
if (!val.empty() && val[0] == ' ')
    val.erase(0, 1);
if (!val.empty() && val[val.length() - 1] == '\r')
    val.resize(val.length() - 1);

Los valores pueden venir con espacios o con un \r al final, así que los
quitamos.

val.erase(0, 1)
    Elimina 1 carácter desde la posición 0.
    Es decir, borra el primer carácter de la cadena.
    Se usa aquí para eliminar el espacio que suele ir tras los dos puntos : en
las cabeceras.

val.resize(val.length() - 1)
    Elimina el último carácter de la cadena.
    Se usa para eliminar el '\r' (retorno de carro) que queda al final de cada
línea HTTP (porque las líneas acaban en \r\n).

y !val.empty()
    Antes de tocar la cadena, siempre se comprueba que no esté vacía, para
evitar errores o undefined behavior si se accede a val[0] o val[val.length() -
1].

9. Guardar en el mapa de cabeceras
_headers[key] = val;
Guardamos el header en un mapa (std::map<std::string, std::string>), para luego
poder acceder fácilmente a cualquier valor

10. detectar cabeceras importantes
if (strcasecmp(key.c_str(), "Content-Length") == 0)
    _contentLength = std::atoi(val.c_str());
else if (strcasecmp(key.c_str(), "Transfer-Encoding") == 0 && val == "chunked")
    _isChunked = true;

    Content-Length indica el tamaño del body, útil si es una petición POST o
PUT.

    Transfer-Encoding: chunked indica que el body vendrá en trozos (“chunks”),
así que se procesa de otra forma.

Usa strcasecmp porque las cabeceras HTTP no distinguen mayúsculas/minúsculas
(Content-Length, content-length, etc. son lo mismo).

strcasecmp(a, b)
    Función de C (definida en <strings.h>) que compara dos char* sin distinguir
mayúsculas/minúsculas.

    Devuelve 0 si son iguales (ignorando el caso).

    Devuelve un valor negativo o positivo si son distintas.

¿Por qué .c_str()?
    Porque strcasecmp trabaja con C-strings (const char*), no con objetos
std::string. Entonces hay que convertir el std::string key a const char* usando
.c_str()
*/

/*
APUNTES ANTIGUOS:
parseHeaders() detecta si llegaron las cabeceras completas (\r\n\r\n).

Si encuentra el delimitador \r\n\r\n, significa que la cabecera HTTP está
completa (ya se ha recibido la petición entera)

➤ Por qué es necesario:
    Las peticiones HTTP no siempre llegan de una sola vez.
    Un cliente puede enviar una parte ahora y otra dentro de unos milisegundos.
    Este método permite leer de forma incremental hasta tener la petición
completa. Como el cliente puede seguir enviando fragmentos parciales, si no
encontramos la secuencia final, devolveremos false.

Si encontramos el final del header:
    Primero guardamos la parte de la cabecera
    Buscamos si dentro del header hay Content-Length para saber si hay body.
        Si lo hay:
            Esto indica que la petición tiene un cuerpo (por ejemplo, un POST o
PUT). Leemos el valor numérico y comprobamos si el tamaño actual del buffer ya
contiene cabecera + body completo. Entonces: size_t endLine =
headerPart.find("\r\n", contentLengthPos); -> Busca el final de esa línea (\r\n)

                std::string value = headerPart.substr(contentLengthPos + 15,
endLine - (contentLengthPos + 15)); -> Corta el trozo de texto que está entre
"Content-Length:"(por eso el +15 caracteres) y el salto de línea → o sea, el
número. find() apunta a la posicion de inicio de lo que buscas, por eso cuando
queremos encontrar el numero hay que sumar 15 caracteres, que son los que tiene
exactamente Content-Length

                _contentLength = std::atoi(value.c_str()); -> Convierte ese
número en entero (std::atoi) y lo guarda en _contentLength.

        Si no hay cabecera "Content-Length:", asumimos que no hay cuerpo (body),
por lo tanto _contentLenght será 0
*/

bool HttpRequest::parseBody(const std::string &rawRequest) {
  // Localizamos el inicio del body: justo después de "\r\n\r\n"
  size_t bodyStart = rawRequest.find("\r\n\r\n");
  if (bodyStart == std::string::npos)
    return false; // No se han recibido todos los headers aún
  bodyStart += 4; // Saltar "\r\n\r\n" (4 caracteres)

  // Nota: El límite de tamaño del body ahora se verifica en RequestHandler
  // usando el valor de client_max_body_size del config
  // (location.getMaxBodySize())

  if (_isChunked) {
    // Chunked encoding: el cliente envía el body en trozos
    // Formato: "5\r\nhello\r\n0\r\n\r\n" (tamaño hex + datos + chunk final 0)
    // Esto es común en POST grandes

    std::string chunkedData = rawRequest.substr(bodyStart);
    return parseChunkedBody(chunkedData);
  }

  // Si hay Content-Length, sabemos exactamente cuántos bytes leer. Esperamos a
  // tener todo el cuerpo
  size_t bodyBytes = rawRequest.size() - bodyStart;
  if (bodyBytes < static_cast<size_t>(_contentLength))
    return false; // aún falta data. Si no tenemos todavía todos los bytes del
                  // body, volvemos al bucle y esperamos a la siguiente vuelta

  // Si llegamos aquí, ya tenemos todo
  // Guardamos el cuerpo completo
  _body = rawRequest.substr(bodyStart, _contentLength);
  _parsedBytes += _contentLength;
  return true;
}

/*
¿Por qué es importante?
    POST envía datos en el body (formularios, archivos, JSON)
    Necesitamos leer exactamente los bytes que el cliente envió
    Content-Length nos dice cuántos bytes esperar

size_t bodyStart = _request.find("\r\n\r\n") + 4;
    _request contiene toda la petición recibida hasta ahora, incluyendo headers
y body. find("\r\n\r\n") devuelve la posición del primer \r\n\r\n, es decir, el
final de los headers. +4 → avanzamos justo después del \r\n\r\n, que es donde
empieza el body.

size_t bodyBytes = _request.size() - bodyStart;
    Calculamos cuántos bytes de body ya hemos recibido.
    _request.size() → total de datos que tenemos
    bodyStart → posición donde empieza el body
    bodyBytes = cantidad de bytes del body que ya llegaron.

if (bodyBytes < static_cast<size_t>(_contentLength))
    return false;

    _contentLength → lo que el cliente dijo que iba a enviar en la cabecera
Content-Length. Si aún no tenemos todos los bytes del body, devolvemos false.
    Esto indica al servidor: “no he terminado de leer la petición; vuelve a
llamar cuando llegue más data”.

    📌 Aquí no hacemos bucles: la función solo revisa si ya está todo, y si no,
se sale.

*/

bool HttpRequest::isKeepAlive() const { return _keepAlive; }

const std::string &HttpRequest::getMethod() const { return _method; }

const std::string &HttpRequest::getPath() const { return _path; }

const std::string &HttpRequest::getQuery() const { return _query; }

const std::string &HttpRequest::getVersion() const { return _version; }

const std::string &HttpRequest::getBody() const { return _body; }
const std::map<std::string, std::string> &HttpRequest::getHeaders() const {
  return _headers;
}

bool HttpRequest::headersComplete() const { return _headersComplete; }
/*
const std::string &HttpRequest::getSpecificHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}
*/
bool HttpRequest::isChunked() const { return _isChunked; }

int HttpRequest::getContentLength() const { return _contentLength; }

int HttpRequest::getParsedBytes() const { return _parsedBytes; }

void HttpRequest::reset() {
  _headersComplete = false;
  _isChunked = false;
  _keepAlive = false;
  _isMalformed = false;
  _parsedBytes = 0;
  _contentLength = -1;
  _method.clear();
  _path.clear();
  _query.clear();
  _version.clear();
  _headers.clear();
  _body.clear();
}

std::string HttpRequest::getOneHeader(const std::string &key) const {
  std::string lowerKey = key;
  for (size_t i = 0; i < lowerKey.length(); ++i) {
    if (lowerKey[i] >= 'A' && lowerKey[i] <= 'Z')
      lowerKey[i] = lowerKey[i] - 'A' + 'a';
  }
  std::map<std::string, std::string>::const_iterator it =
      _headers.find(lowerKey);
  if (it != _headers.end()) {
    return it->second;
  }
  return "";
}

/**
 * @brief Parsea un body en formato chunked y lo convierte en body normal.
 *
 * Formato chunked:
 *   <tamaño en hex>\r\n
 *   <datos>\r\n
 *   ...
 *   0\r\n
 *   \r\n
 *
 * @param chunkedData Los datos raw después de los headers
 * @return true si el chunked está completo, false si faltan datos
 */
bool HttpRequest::parseChunkedBody(const std::string &chunkedData) {
  std::string result;
  size_t pos = 0;

  while (pos < chunkedData.size()) {
    // 1. Buscar el fin de la línea del tamaño (\r\n)
    size_t lineEnd = chunkedData.find("\r\n", pos);
    if (lineEnd == std::string::npos) {
      return false; // Datos incompletos, esperar más
    }

    // 2. Extraer el tamaño del chunk (hexadecimal)
    std::string chunkSizeStr = chunkedData.substr(pos, lineEnd - pos);

    // Ignorar extensiones de chunk (después de ';') si las hubiera
    size_t semicolon = chunkSizeStr.find(';');
    if (semicolon != std::string::npos) {
      chunkSizeStr = chunkSizeStr.substr(0, semicolon);
    }

    // Convertir hex a entero
    char *endPtr;
    long chunkSize = std::strtol(chunkSizeStr.c_str(), &endPtr, 16);

    if (endPtr == chunkSizeStr.c_str() || chunkSize < 0) {
      // Error de parseo del tamaño
      std::cerr << "[Error] Chunked: tamaño inválido '" << chunkSizeStr
                << "'\n";
      return false;
    }

    // 3. Es el chunk final? Chunk de tamaño 0 = fin del body
    if (chunkSize == 0) {
      _body = result;             // guardamos el body completo acumulado
      _parsedBytes = lineEnd + 4; // pos + 0\r\n\r\n
      return true;
    }

    // 4. Verificar que tenemos suficientes datos
    size_t dataStart = lineEnd + 2; // saltamos el \r\n
    size_t chunkLen = static_cast<size_t>(chunkSize);

    if (dataStart + chunkLen + 2 > chunkedData.size()) {
      return false; // No tenemos el chunk completo, esperar más datos
    }

    // 5. Extraer los datos del chunk
    result.append(chunkedData, dataStart, chunkLen);

    // 7. Avanzar al siguiente chunk (+2 para saltar \r\n después de los datos)
    pos = dataStart + chunkLen +
          2; // pos ahora apunta al inicio del siguiente chunk
  }

  // Si llegamos aquí, no hemos encontrado el chunk final (0\r\n)
  return false;
}
