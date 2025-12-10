#pragma once

#include <string>

class Client; // forward declaration (declaración adelantada) de la clase Client. Usamos esto en vez de #include "Client.hpp" sobretodo para evitar dependencias circulares

namespace Autoindex
{
    void handleDirectory(Client *client, const std::string &dirPath, const std::string &urlPath, bool autoindexEnabled, const std::string &defaultFile);

    void generateListing(Client *client, const std::string &dirPath, const std::string &urlPath);

    // Funciones helper de seguridad
    std::string escapeHtml(const std::string &input);
    std::string urlEncode(const std::string &input);
}
/*
Un namespace es simplemente una forma de agrupar funciones bajo un nombre propio.
Ventajas:
✔️ Evita colisiones de nombres
    No tendrás dos funciones llamadas generateHTML() que se pisen.

✔️ Estructura tu proyecto
    Sabes que todo lo que tiene que ver con autoindex está dentro de:

namespace Autoindex { ... }

✔️ No necesita objetos ni clases
    Ideal para funciones puras que no requieren estado interno.

quiero me explique al detalle la funcion y su implementacion, linea por linea, que es todo y que hace
*/

/*
¿Qué es el autoindex?
    El autoindex es una funcionalidad típica de servidores web (como Nginx o Apache) que genera automáticamente una lista de archivos y directorios cuando el usuario accede a una carpeta y no hay un index.html.

    Ejemplo:

    Si el usuario navega a:
        http://localhost:8080/uploads/


    Y dentro de www/uploads/ tienes:
        file1.txt
        file2.png
        folderA/

    Entonces el servidor genera automáticamente una página HTML como:

        Index of /uploads

        file1.txt
        file2.png
        folderA/


    Con enlaces para descargarlos.

🧠 ¿Para qué sirve?

    Sirve para navegar por el filesystem web de manera simple:
        Ver archivos subidos
        Ver contenido de un directorio
        Descargarlos
        Ver estructura

    Es especialmente útil en Webserv para probar fácilmente los uploads y DELETE.

📌 Para el proyecto, el servidor debe soportar autoindex ON/OFF

        Si el autoindex está activado y no hay index.html en un directorio → mostrar listado

        Si el autoindex está desactivado → devolver 403 Forbidden

    Y debe configurarse desde el fichero .conf

📁 ¿Cómo se comporta según el subject?

    Según Webserv:

    | Caso                                      | Resultado                |
    | ----------------------------------------- | ------------------------ |
    | Directorio con `index.html`               | **Servir index.html**    |
    | Directorio sin index.html + autoindex ON  | **Generar listado HTML** |
    | Directorio sin index.html + autoindex OFF | **403 Forbidden**        |
    | Acceso a archivo                          | **Servir archivo**       |
    | Acceso a path que no existe               | **404 Not Found**        |


🛠 ¿Cómo implementarlo técnicamente?

    Cuando detectas que la solicitud apunta a un directorio, en serveStaticFile() o en handleGet() debes:

        1. Detectar si es directorio
            if (S_ISDIR(fileStat.st_mode)) { ... }

        2. Comprobar si hay un index.html dentro
            std::string indexPath = fullPath + "/index.html";
            stat(indexPath.c_str(), &indexStat)
        Si existe → lo sirves.

        3. Si no existe index.html → comprobar si autoindex está activado
            Eso depende de tu configuración.

            Ejemplo:
                location /uploads
                {
                    autoindex on;
                }

            Entonces en tu código tendrás:
                if (config.autoindex == true)
                    return generateAutoindex(fullPath);
                else
                    return 403;

        4. Generar la página HTML del autoindex
            Basta con iterar con opendir() + readdir():
                DIR *dir = opendir(fullPath.c_str());
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL)
                {
                    // entry->d_name → nombre del archivo
                }


            Y crear una respuesta HTML simple:
                <html>
                <body>
                <h1>Index of /uploads</h1>
                <ul>
                <li><a href="file1.txt">file1.txt</a></li>
                <li><a href="image.png">image.png</a></li>
                </ul>
                </body>
                </html>

🚀 Qué aporta?
    Aprender a:
        leer el filesystem
        construir HTML dinámico
        entiender bien los códigos de estado 403/404
        gestionar bien rutas de directorios
        manejar bien stat(), opendir(), readdir()
        controlar la lógica de routes en un servidor

📝 Resumen corto
    Autoindex genera automáticamente una página HTML con el listado de archivos de un directorio cuando el usuario accede a una carpeta que no tiene index.html. Si está activado por configuración, se muestra el listado; si está desactivado, se debe devolver 403 Forbidden. Forma parte obligatoria del proyecto Webserv.


🧠 Autoindex NO crea nada. Simplemente muestra lo que ya existe.

    No modifica el filesystem, no crea archivos, no crea carpetas.

    Lo único que hace es:
        👉 Leer el contenido real de un directorio en tu disco (con opendir/readdir)
        👉 Crear una página HTML que lista esos archivos y subdirectorios
        👉 Mandar esa página al navegador

    Y el navegador te la muestra como una página web con enlaces.

🔥 Ejemplo concreto: imaginemos que tienes esta estructura en tu disco
    www/
    ├── index.html
    ├── uploads/
    │    ├── photo.png
    │    ├── resume.pdf
    │    └── notes.txt
    └── other/
        └── data.json

    Tú no creas esto desde autoindex, esto ya existe en tu máquina física.

🌐 Accedes en el navegador a:
    http://localhost:8080/uploads/

Tu servidor hace:
    Ve que /uploads/ es un directorio
    Busca /uploads/index.html → no existe
    Pregunta a la config → ¿autoindex ON?
    Genera HTML como:
        <html>
        <body>
        <h1>Index of /uploads</h1>
        <ul>
            <li><a href="photo.png">photo.png</a></li>
            <li><a href="resume.pdf">resume.pdf</a></li>
            <li><a href="notes.txt">notes.txt</a></li>
        </ul>
        </body>
        </html>
    Te lo envía al navegador

🖱️ ¿Cómo "navega" el usuario?
    Ahora verás esto en tu navegador como una página con enlaces.

    Si haces click en photo.png:
        http://localhost:8080/uploads/photo.png

    → tu servidor servirá ese archivo.

    Si haces click en notes.txt:
        http://localhost:8080/uploads/notes.txt

    → tu servidor envía ese texto.

    Así es como “navegas” el filesystem:
    simplemente porque autoindex genera enlaces a los archivos reales.


🧭 Navegación real — NO genera nada nuevo
    Es importante entender:
        El servidor no crea archivos ni carpetas
        Solo lee lo que ya existe
        Y el navegador te deja ir siguiendo enlaces como si fuera un explorador web

    Es como cuando en tu ordenador usas Finder/Explorer, pero aquí la visualización se hace con una página HTML generada automáticamente.

🎯 ¿Por qué es útil?
    Para probar tus uploads (POST)
    Para poder borrar archivos con DELETE desde el navegador
    Para ver qué hay en una carpeta sin necesidad de un archivo index.html
    Para depuración rápida

🔑 Analogía rápida
    Autoindex = un “explorador de archivos” muy básico hecho en HTML.
    No crea nada, solo muestra lo que ya está en la carpeta real del servidor.

*/