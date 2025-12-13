#ifndef CGIEXECUTOR_HPP
#define CGIEXECUTOR_HPP

#include <string>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>
#include <cstdlib>

class CGIExecutor
{
private:
    int _pipeIn[2];
    int _pipeOut[2];
    pid_t _childPid;

public:
    CGIExecutor();
    ~CGIExecutor();

    std::string execute(const std::string &executable,
                    const std::string &scriptPath,
                    char **envp,
                    const std::string &requestBody);

private:
    void setupPipes();
    void executeChild(const std::string &executable,
                    const std::string &scriptPath,
                    char **envp);
    std::string readChildOutput();
    void writeToChild(const std::string &data);
};

#endif