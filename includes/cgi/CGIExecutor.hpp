#ifndef CGIEXECUTOR_HPP
#define CGIEXECUTOR_HPP

#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

/** @brief Result of async CGI execution */
struct CGIAsyncResult
{
  int pipeFd;     // Pipe to read CGI output (non-blocking)
  pid_t childPid; // PID of forked CGI process
  bool success;   // true if fork succeeded
};

/**
 * @brief CGI process executor - handles fork, exec, and pipe I/O
 */
class CGIExecutor
{
private:
  int _pipeIn[2];
  int _pipeOut[2];
  pid_t _childPid;

public:
  CGIExecutor();
  ~CGIExecutor();

  /** @brief Synchronous execution - blocks until CGI completes */
  std::string execute(const std::string &executable,
                      const std::string &scriptPath, char **envp,
                      const std::string &requestBody);

  /** @brief Async execution - forks but doesn't wait */
  CGIAsyncResult executeAsync(const std::string &executable,
                              const std::string &scriptPath, char **envp,
                              const std::string &requestBody);

  static void setNonBlocking(int fd);

private:
  void setupPipes();
  void executeChild(const std::string &executable,
                    const std::string &scriptPath, char **envp);
  std::string readChildOutput();
  void writeToChild(const std::string &data);
};

#endif