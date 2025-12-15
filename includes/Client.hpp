#pragma once

#include <string>
#include <netinet/in.h> // sockaddr_in
#include <unistd.h>     // close()
#include <ctime>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <vector>
#include <string.h>

// HARDCODEADO, va en configfile
#define WWW_ROOT "./www" // raíz de los archivos web

/*
Define un document root donde estarán tus archivos web. Por ejemplo:
    WWW_ROOT "./www"


Si recibes /index.html, el servidor busca:
    ./www/index.html

Si recibes /css/style.css → busca ./www/css/style.css.

Esto evita que sirvas archivos del sistema fuera de tu carpeta web.
*/

class Client
{
public:
    Client(int fd, const sockaddr_in &addr);
    ~Client();

    int getFd() const;
    std::string getIp() const;

    bool readRequest();    // lee datos del cliente
    bool processRequest(); // crea HttpResponse basado en HttpRequest
    bool sendResponse();   // envía respuesta
    bool isClosed() const;

    // nuevo: encolar respuesta y vaciar buffer progresivamente
    bool flushWrite();            // intenta enviar bytes pendientes (usa send())
    bool hasPendingWrite() const; // true si queda data por enviar
    void markClosed();
    bool isRequestComplete() const; // TODO: BORRAR????
    const HttpRequest &getHttpRequest() const;

    // timeout helpers
    time_t getLastActivity() const;
    bool isTimedOut(time_t now, int timeoutSec) const;

    // preparar para la próxima request cuando hay keep-alive
    void resetForNextRequest();

    void serveStaticFile(const std::string &fullPath);

    // métodos helper para Autoindex
    bool sendHtmlResponse(const std::string &html);
    bool sendError(int errorCode);

    //! TEMPORAL:
    // Método temporal para obtener configuración (hasta que terminemos todo lo de config)
    struct TempRouteConfig
    {
        bool autoindex;
        std::string defaultFile;
    };
    TempRouteConfig getTempRouteConfig(const std::string &path);
    // Finalidad de la estructura:
    // ✔ Decide si autoindex está activado para esa ruta
    // ✔ Decide qué archivo usar como “default” (index.html)

private:
    int _clientFd;     // file descriptor del socket del cliente
    sockaddr_in _addr; // dirección IP y puerto del cliente   // TODO *****SI DEJO ESTE PONER IGUAL EN EL SERVER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bool _closed;      // indica si la conexión está cerrada

    // lectura/parseo

    std::string _rawRequest; // buffer con los datos RAW recibidos del socket
                             // Client es responsable del socket y de recibir bytes.
                             // → Por tanto, debe guardar temporalmente lo que va llegando desde el socket

    // bool _keepAlive;      // para saber si la conexión se debe mantener viva después de mandar una petición o no
    HttpRequest _httpRequest;
    // HttpRequest no sabe nada del socket. Solo sabe parsear texto una vez lo tiene completo.
    // → Su función es transformar texto crudo → estructura interpretada (método, headers, body...).

    //  salida (write buffering)
    std::string _writeBuffer; // Los datos pendientes de enviar
    size_t _writeOffset;      // bytes ya enviados desde el inicio de _writeBuffer. indica cuánto ya has enviado — así no reenvías bytes ya enviados.
    time_t _lastActivity;     // timestamp del último recv/send exitoso. te permitirá implementar timeouts (más tarde)

    bool _requestComplete;

    HttpResponse _httpResponse;

    // Límite razonable para servir archivos en memoria. Ajustar según recursos.
    static const size_t MAX_STATIC_FILE_SIZE = 10 * 1024 * 1024; // TODO: 10 MB SE PUEDE DECLARAR ASI A PELO?
    /*Propósito: Límite cuando tú SIRVES archivos a clientes
        Ejemplo: Cliente pide GET /big-video.mp4
            Tú lees el archivo del disco y lo envías
            Protege tu RAM al leer archivos grandes
*/

    // Helpper
    void applyConnectionHeader();
    bool validateMethod();
    bool handleGet();
    bool handleHead();
    bool handlePost();
    bool handleDelete();
    std::string urlDecode(const std::string &encoded, bool plusAsSpace) const;
    std::string getDecodedPath() const;
    std::string getDecodedQuery() const;
    std::string sanitizePath(const std::string &path);
    std::string buildFullPath(const std::string &cleanPath);
    bool readFileToString(const std::string &fullPath, std::string &out, size_t size); // Helpper para serveStaticFile

    std::string determineMimeType(const std::string &path);

    // HARDCODEADO, va en configfile -> Funciona como lookup rápido para saber qué Content-Type poner según extensión
    std::map<std::string, std::string> mimeTypes;
};

/*
Por qué la necesitamos
    Hasta ahora, cada cliente era solo un número (int clientFd).
    Pero pronto querremos almacenar más cosas por cada cliente:

        Su dirección IP.

        Lo que ha enviado.

        El estado actual (leyendo, esperando respuesta, cerrado...).

        Quizás un buffer parcial si la solicitud llega por trozos.

    Por eso conviene crear una clase Client que represente a cada cliente conectado.


    Explicación:

        _clientFd → es el identificador del socket que representa a este cliente.

        _addr → contiene la dirección IP y el puerto del cliente (lo llena accept()).

        _buffer → servirá para almacenar lo que el cliente envía (por si llega por trozos).

        _closed → nos permite marcar si el cliente ya cerró la conexión, y así poder eliminarlo del poll() más tarde.
*/

/*
¿Dónde declarar HttpRequest _httpRequest?

    👉 En la clase Client, como miembro privado.

    Por qué:
        Cada Client representa una conexión individual, por tanto, su HttpRequest también es única.

        Nadie fuera de Client debería modificar los datos crudos del request, solo leerlos.

        Desde fuera (por ejemplo, en Server), accederás a los datos a través de getters o referencias controladas, no modificando directamente _httpRequest.
*/