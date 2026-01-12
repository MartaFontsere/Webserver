#ifndef CGIEXECUTOR_HPP
#define CGIEXECUTOR_HPP

#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Result of async CGI execution containing pipe FD and child PID
 */
struct CGIAsyncResult {
  int pipeFd;     // Pipe to read CGI output (already non-blocking)
  pid_t childPid; // PID of forked CGI process
  bool success;   // true if fork succeeded
};

class CGIExecutor {
private:
  int _pipeIn[2];
  int _pipeOut[2];
  pid_t _childPid;

public:
  CGIExecutor();
  ~CGIExecutor();

  // Original synchronous execute (for backwards compatibility)
  std::string execute(const std::string &executable,
                      const std::string &scriptPath, char **envp,
                      const std::string &requestBody);

  // NEW: Async execute - forks but doesn't wait, returns pipe FD
  CGIAsyncResult executeAsync(const std::string &executable,
                              const std::string &scriptPath, char **envp,
                              const std::string &requestBody);

  // NEW: Set file descriptor to non-blocking
  static void setNonBlocking(int fd);

private:
  void setupPipes();
  void executeChild(const std::string &executable,
                    const std::string &scriptPath, char **envp);
  std::string readChildOutput();
  void writeToChild(const std::string &data);
};

#endif