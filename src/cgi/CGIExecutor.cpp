#include "../../includes/cgi/CGIExecutor.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

CGIExecutor::CGIExecutor()
{
}

CGIExecutor::~CGIExecutor()
{
}

void CGIExecutor::setupPipes()
{
    pipe(_pipeIn);
    pipe(_pipeOut);
}

std::string CGIExecutor::execute(const std::string &executable,
                            const std::string &scriptPath, char **envp,
                            const std::string &requestBody)
{
    setupPipes();

    _childPid = fork();

    if (_childPid < 0)
        throw std::runtime_error("Failed to fork CGI process");

    if (_childPid == 0)
        executeChild(executable, scriptPath, envp);

    close(_pipeIn[0]);
    close(_pipeOut[1]);

    writeToChild(requestBody);
    close(_pipeIn[1]);
    std::string output = readChildOutput();
    close(_pipeOut[0]);
    waitpid(_childPid, NULL, 0);

    return output;
}

void CGIExecutor::executeChild(const std::string &executable,
                        const std::string &scriptPath, char **envp)
{
    dup2(_pipeIn[0], STDIN_FILENO);
    dup2(_pipeOut[1], STDOUT_FILENO);

    close(_pipeOut[1]);
    close(_pipeIn[1]);
    close(_pipeIn[0]);
    close(_pipeOut[0]);

   // size_t lastSlash = scriptPath.find_last_of('/');
   // std::string directory = scriptPath.substr(0, lastSlash);
    //std::string fileName = scriptPath.substr(lastSlash + 1);

    char **argv = new char*[3];
    argv[0] = new char[executable.size() + 1];
    stringToCString(executable, argv[0]);
    argv[1] = new char[scriptPath.size() + 1];
    stringToCString(scriptPath, argv[1]);
    argv[2] = NULL;

    //chdir(directory.c_str());
    execve(argv[0], argv, envp);
    delete[] argv[0];
    delete[] argv[1];
    delete[] argv;
    exit(1);
}

void CGIExecutor::writeToChild(const std::string &data)
{
    if (!data.empty())
        write(_pipeIn[1], data.c_str(), data.size());

}

std::string CGIExecutor::readChildOutput()
{
    std::string result;
    char buffer[4096];

    while (true)
    {
        ssize_t bytesRead = read(_pipeOut[0], buffer, 4096);

        if (bytesRead <= 0)
            break;

        result.append(buffer, bytesRead);
    }

    return result;
}






