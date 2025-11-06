#pragma once

#include <string>
#include <netinet/in.h> // sockaddr_in
#include <unistd.h>     // close()
#include <ctime>

class Client
{
public:
    Client(int fd, const sockaddr_in &addr);
    ~Client();

    int getFd() const;
    std::string getIp() const;

    bool readRequest();                        // lee datos del cliente
    bool sendResponse(const std::string &msg); // envía respuesta
    bool isClosed() const;

    // nuevo: encolar respuesta y vaciar buffer progresivamente
    bool queueResponse(const std::string &msg); // añade msg a _writeBuffer y llama a flushWrite()
    bool flushWrite();                          // intenta enviar bytes pendientes (usa send())
    bool hasPendingWrite() const;               // true si queda data por enviar
    void markClosed();                          // 👈 NUEVA FUNCIÓN PÚBLICA

private:
    int _clientFd;     // file descriptor del socket del cliente
    sockaddr_in _addr; // dirección IP y puerto del cliente   *****SI DEJO ESTE PONER IGUAL EN EL SERVER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bool _closed;      // indica si la conexión está cerrada

    // lectura/parseo
    bool _headersComplete;
    bool _requestComplete;
    int _contentLength;
    std::string _request; // buffer con los datos recibidos del header
    std::string _body;    // buffer con los datos recibidos del body
    // bool _keepAlive;      // para saber si la conexión se debe mantener viva después de mandar una petición o no

    //  salida (write buffering)
    std::string _writeBuffer; // todo lo pendiente por enviar
    size_t _writeOffset;      // bytes ya enviados desde el inicio de _writeBuffer. indica cuánto ya has enviado — así no reenvías bytes ya enviados.
    time_t _lastActivity;     // timestamp del último recv/send exitoso. te permitirá implementar timeouts (más tarde)

    // parsing helpers
    bool parseHeaders();
    bool parseBody();
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