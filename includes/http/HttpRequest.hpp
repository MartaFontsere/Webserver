#pragma once

#include <map>
#include <string>

class HttpRequest {
public:
  HttpRequest();
  bool
  parse(const std::string &rawRequest); // devuelve true si request completa

  // Getters útiles
  const std::string &getMethod() const;
  const std::string &getPath() const;
  const std::string &getQuery() const;
  const std::string &getVersion() const;
  const std::string &getBody() const;
  const std::map<std::string, std::string> &getHeaders() const;
  std::string getOneHeader(const std::string &key) const;
  int getParsedBytes() const;

  bool headersComplete() const;
  bool isChunked() const;
  bool isKeepAlive() const;
  bool isMalformed() const;
  int getContentLength() const;

  // resetea el estado para reutilizar el objeto
  void reset();

private:
  // estado interno
  bool _headersComplete;
  bool _isChunked;
  bool _keepAlive;   // true → mantener abierta
  bool _isMalformed; // true → error en el parseo
  int _parsedBytes;  // número de bytes consumidos del rawRequest por el parse

  // elementos del request
  std::string _method;
  std::string _path;  // SOLO path
  std::string _query; // Lo que va tras '?'
  std::string _version;
  std::map<std::string, std::string> _headers;
  std::string _body; // buffer con los datos recibidos del body

  // control del body
  int _contentLength;

  // parsing helpers
  bool parseHeaders(const std::string &rawRequest);
  bool parseBody(const std::string &rawRequest);
  bool parseChunkedBody(const std::string &chunkedData);
  std::string _urlDecode(const std::string &encoded, bool plusAsSpace) const;
};

/*
HttpRequest no debe conocer Client. HttpRequest es sólo lógica HTTP (método,
headers, body). Los logs sobre FD o sobre la conexión se hacen en Client.

HttpRequest::parse(const std::string &raw) toma la entrada cruda y devuelve true
sólo si la petición (headers + body) está completa y parseada. No hace close()
ni nada de sockets.

HttpRequest mantiene su propio estado interno: _headersComplete, _isChunked,
_contentLength, _method, _path, _version, _headers, _body.

Client es quien llama a parse() y quien hace logs usando su _clientFd. Si
quieres logs del parser, HttpRequest puede devolver códigos de estado o dejar
que Client haga los mensajes.
*/