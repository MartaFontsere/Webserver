#pragma once

#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/RequestHandler.hpp"
#include <ctime>
#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <vector>

/** @brief CGI process state for this connection */
enum CGIState {
  CGI_NONE,    // No CGI running
  CGI_RUNNING, // CGI fork() executed, waiting for output
  CGI_DONE     // CGI finished, response ready to send
};

/**
 * @brief Individual client connection - manages request/response lifecycle
 */
class ClientConnection {
public:
  ClientConnection(int fd, const sockaddr_in &addr,
                   const std::vector<ServerConfig> &serverCandidateConfigs);
  ~ClientConnection();

  int getFd() const;
  std::string getIp() const;

  /** @brief Read data from client socket into buffer */
  bool readRequest();
  /** @brief Process buffered request and generate response */
  bool processRequest();
  /** @brief Send response data to client */
  bool sendResponse();
  bool isClosed() const;

  /** @brief Attempt to send pending write buffer (non-blocking) */
  bool flushWrite();
  bool hasPendingWrite() const;
  void markClosed();
  bool isRequestComplete() const;
  const HttpRequest &getHttpRequest() const;

  // Timeout helpers
  time_t getLastActivity() const;
  void updateActivity();
  bool isTimedOut(time_t now, int timeoutSec) const;

  // Keep-alive support
  void resetForNextRequest();
  bool checkForNextRequest();

  // CGI non-blocking API
  CGIState getCGIState() const;
  int getCGIPipeFd() const;
  pid_t getCGIPid() const;
  void startCGI(int pipeFd, pid_t pid);
  bool readCGIOutput();
  void finishCGI(int exitStatus);
  const std::string &getCGIBuffer() const;
  void setCGIResponse(const std::string &responseStr);

private:
  int _clientFd;
  sockaddr_in _addr;
  bool _closed;

  std::string _rawRequest;
  HttpRequest _httpRequest;

  std::string _writeBuffer;
  size_t _writeOffset;
  time_t _lastActivity;
  bool _requestComplete;
  std::vector<ServerConfig> _servCandidateConfigs;

  HttpResponse _httpResponse;
  RequestHandler _requestHandler;

  CGIState _cgiState;
  int _cgiPipeFd;
  pid_t _cgiPid;
  std::string _cgiBuffer;
};
