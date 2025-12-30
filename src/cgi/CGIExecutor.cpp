#include "../../includes/cgi/CGIExecutor.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

/**
 * @file CGIExecutor.cpp
 * @brief CGI script execution via fork/exec/pipes (POSIX compliant)
 *
 * This module handles the actual execution of CGI scripts as separate processes
 * using POSIX system calls (fork, execve, pipe, dup2). It creates isolated
 * child processes that run CGI interpreters (php-cgi, python3, etc.) and
 * captures their output through inter-process communication pipes.
 *
 * Process architecture:
 *   PARENT (webserver)              CHILD (CGI script)
 *   ==================              ==================
 *   write(_pipeIn[1])  ────────────> read(STDIN)
 *   read(_pipeOut[0])  <──────────── write(STDOUT)
 *
 * Pipe structure:
 * - _pipeIn[2]:  Parent writes POST data → Child reads from stdin
 *   └─ _pipeIn[0] = read end (child's stdin)
 *   └─ _pipeIn[1] = write end (parent writes here)
 *
 * - _pipeOut[2]: Child writes output → Parent reads result
 *   └─ _pipeOut[0] = read end (parent reads here)
 *   └─ _pipeOut[1] = write end (child's stdout)
 *
 * Execution flow:
 * 1. setupPipes() - Create communication channels
 * 2. fork() - Split into parent/child processes
 * 3. Child: dup2() redirects stdin/stdout, execve() becomes CGI script
 * 4. Parent: writes POST data, reads script output, waits for child termination
 * 5. Return captured output to caller
 *
 * Key design decisions:
 * - No chdir(): Uses absolute paths in argv[1] (universal for all CGI types)
 * - 4KB buffer: POSIX page size (optimal for read/write operations)
 * - Ordered pipe closing: Parent closes unused ends immediately
 * - Zombie prevention: waitpid() after reading output
 *
 * @note All syscalls are POSIX standard (no C++ alternatives exist)
 * @see POSIX.1-2001 for fork, execve, pipe, dup2 specifications
 */

/**
 * @brief Default constructor
 *
 * Initializes a CGIExecutor object with uninitialized pipe file descriptors.
 * Pipes are created later in setupPipes() when execute() is called.
 *
 * Member initialization:
 * - _pipeIn[2]: Uninitialized (will be set by pipe() syscall)
 * - _pipeOut[2]: Uninitialized (will be set by pipe() syscall)
 * - _childPid: Uninitialized (will be set by fork())
 *
 * @note Constructor is empty because initialization happens in execute()
 */
CGIExecutor::CGIExecutor() {}

/**
 * @brief Destructor
 *
 * Cleans up the CGIExecutor object. No explicit cleanup needed because:
 * - File descriptors are closed within execute() method
 * - Child process is waited for with waitpid() (no zombie processes)
 * - No dynamic memory allocated as member variables
 *
 * @note Destructor is empty because all cleanup is handled in execute()
 */
CGIExecutor::~CGIExecutor() {}

/**
 * @brief Creates two pipes for bidirectional communication with child process
 *
 * Initializes the pipe file descriptors that will be used for communication
 * between parent (webserver) and child (CGI script) processes.
 *
 * Pipe creation:
 * - pipe(_pipeIn)  → Creates stdin channel for child
 *   └─ _pipeIn[0]: Read end (child reads POST data)
 *   └─ _pipeIn[1]: Write end (parent writes POST data)
 *
 * - pipe(_pipeOut) → Creates stdout channel for child
 *   └─ _pipeOut[0]: Read end (parent reads script output)
 *   └─ _pipeOut[1]: Write end (child writes output)
 *
 * After fork():
 * - Child uses: _pipeIn[0] (stdin), _pipeOut[1] (stdout)
 * - Parent uses: _pipeIn[1] (write data), _pipeOut[0] (read results)
 *
 * @throws std::runtime_error if pipe() syscall fails (e.g., too many open
 * files)
 *
 * @note Called at the beginning of execute() before fork()
 * @note Pipes are unidirectional - each direction requires separate pipe
 */
void CGIExecutor::setupPipes() {
  pipe(_pipeIn);
  pipe(_pipeOut);
}

/**
 * @brief Executes a CGI script and captures its output (orchestrator method)
 *
 * This is the main entry point for CGI execution. It orchestrates the entire
 * process of running a CGI script: creating pipes, forking, managing I/O,
 * and collecting the result.
 *
 * Execution sequence:
 * 1. setupPipes() - Create communication channels
 * 2. fork() - Create child process
 *    - Fork failure: throw exception
 *    - Child (pid == 0): Call executeChild() → never returns (execve or exit)
 *    - Parent (pid > 0): Continue to step 3
 * 3. Close unused pipe ends:
 *    - _pipeIn[0]: Child reads from this (parent doesn't need)
 *    - _pipeOut[1]: Child writes to this (parent doesn't need)
 * 4. writeToChild() - Send POST data to child's stdin
 * 5. close(_pipeIn[1]) - Signal EOF to child (no more data)
 * 6. readChildOutput() - Read all output from child's stdout
 * 7. close(_pipeOut[0]) - Done reading
 * 8. waitpid() - Wait for child termination (prevent zombie process)
 * 9. Return captured output
 *
 * Pipe usage diagram:
 *   Parent Process              Child Process
 *   ==============              =============
 *   close(_pipeIn[0])           dup2(_pipeIn[0], STDIN)
 *   write(_pipeIn[1]) --------> read(STDIN)
 *   close(_pipeIn[1]) ----EOF--> (child sees EOF on stdin)
 *
 *   close(_pipeOut[1])          dup2(_pipeOut[1], STDOUT)
 *   read(_pipeOut[0]) <-------- write(STDOUT)
 *   close(_pipeOut[0])          (child exits)
 *
 * Error handling:
 * - Fork failure: Throws runtime_error immediately
 * - Execve failure: Child exits with code 1 (parent receives empty output)
 * - Read/write errors: Handled by syscalls (read returns 0 on EOF)
 *
 * Example usage:
 *   CGIExecutor executor;
 *   std::string output = executor.execute("/usr/bin/php-cgi",
 *                                         "./scripts/hello.php",
 *                                         envp,
 *                                         "name=world");
 *   output contains HTTP headers + HTML body from script
 *
 * @param executable Path to CGI interpreter (e.g., "/usr/bin/php-cgi",
 * "/usr/bin/python3")
 * @param scriptPath Complete filesystem path to script file
 * @param envp NULL-terminated array of environment variables (from
 * CGIEnvironment::toEnvArray())
 * @param requestBody POST/PUT body data to send to script's stdin (empty for
 * GET)
 * @return Complete output from CGI script (headers + body)
 *
 * @throws std::runtime_error if fork() fails
 * @note Blocks until child process completes (synchronous execution)
 * @note Child process becomes zombie until waitpid() is called
 */
std::string CGIExecutor::execute(const std::string &executable,
                                 const std::string &scriptPath, char **envp,
                                 const std::string &requestBody) {
  setupPipes();

  _childPid = fork();

  if (_childPid < 0)
    throw std::runtime_error("Failed to fork CGI process");

  if (_childPid == 0)
    executeChild(executable, scriptPath,
                 envp); // Never returns (execve or exit)
  // Parent process continues here
  close(_pipeIn[0]);  // Parent doesn't read from stdin pipe
  close(_pipeOut[1]); // Parent doesn't write to stdout pipe

  writeToChild(requestBody);
  close(_pipeIn[1]); // Signal EOF to child (important for script termination)
  std::string output = readChildOutput();
  close(_pipeOut[0]); // Cerramos el pipe de lectura tras obtener la salida

  int status;
  // Esperamos a que el proceso hijo (el script) termine para evitar procesos
  // zombie
  waitpid(_childPid, &status, 0);

  // Si el script terminó pero con un código de error (distinto de 0)
  // lanzamos una excepción para que el CGIHandler devuelva un 500 Internal
  // Server Error
  if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    throw std::runtime_error("CGI script exited with error");

  return output;
}

/**
 * @brief Child process code - sets up I/O redirection and executes CGI script
 *
 * This function runs in the child process (after fork). It never returns to
 * the caller because execve() replaces the entire process image. If execve()
 * fails, the child exits with code 1.
 *
 * I/O redirection process:
 * 1. dup2(_pipeIn[0], STDIN_FILENO) - Redirect stdin to read from parent's pipe
 *    → Now read(STDIN) reads from _pipeIn[0]
 * 2. dup2(_pipeOut[1], STDOUT_FILENO) - Redirect stdout to write to parent's
 * pipe → Now write(STDOUT) writes to _pipeOut[1]
 * 3. Close all unnecessary pipe ends (parent will use the others)
 *
 * Pipe closing (child perspective):
 * - close(_pipeOut[1]): Original stdout pipe (now duplicated to STDOUT)
 * - close(_pipeIn[1]):  Write end (only parent uses this)
 * - close(_pipeIn[0]):  Original stdin pipe (now duplicated to STDIN)
 * - close(_pipeOut[0]): Read end (only parent uses this)
 *
 * argv construction:
 *   argv[0] = executable path (e.g., "/usr/bin/php-cgi")
 *   argv[1] = script path (e.g., "./test_scripts/hello.php")
 *   argv[2] = NULL (execve requirement)
 *
 * Why argv[1] is complete path (no chdir):
 * - Universal: Works for PHP, Python, Bash, any interpreter
 * - Simpler: No need to parse path and change directory
 * - Safer: Script can't escape intended directory
 * - RFC 3875: Doesn't mandate chdir() to script directory
 *
 * execve() behavior:
 * - Success: Process image replaced, this function never returns
 * - Failure: Function continues to cleanup code
 *
 * Memory cleanup (only reached if execve fails):
 * - delete[] argv[0] and argv[1] (strings allocated with new)
 * - delete[] argv (array allocated with new)
 * - exit(1) to signal failure to parent
 *
 * @param executable Path to CGI interpreter (becomes argv[0])
 * @param scriptPath Complete path to script file (becomes argv[1])
 * @param envp Environment variables array (prepared by CGIEnvironment)
 *
 * @note This function NEVER returns normally (execve or exit)
 * @note Called only in child process (after fork returns 0)
 * @warning Do not add code after exit(1) - it will never execute
 */
void CGIExecutor::executeChild(const std::string &executable,
                               const std::string &scriptPath, char **envp) {
  // Redirect stdin/stdout to pipes
  dup2(_pipeIn[0], STDIN_FILENO);   // Read POST data from parent
  dup2(_pipeOut[1], STDOUT_FILENO); // Write output to parent
  // Close all pipe file descriptors (no longer needed after dup2)
  close(_pipeOut[1]);
  close(_pipeIn[1]);
  close(_pipeIn[0]);
  close(_pipeOut[0]);
  // Prepare argv for execve
  char **argv = new char *[3];
  argv[0] = new char[executable.size() + 1];
  stringToCString(executable, argv[0]);
  argv[1] = new char[scriptPath.size() + 1];
  stringToCString(scriptPath, argv[1]);
  argv[2] = NULL;
  // Replace process image with CGI interpreter
  execve(argv[0], argv, envp);
  // Only reached if execve fails
  delete[] argv[0];
  delete[] argv[1];
  delete[] argv;
  exit(1);
}

/**
 * @brief Writes POST/PUT request body to child process stdin
 *
 * Sends the HTTP request body to the CGI script through the input pipe.
 * The script reads this data from its stdin (after dup2 redirection).
 *
 * Data flow:
 *   Parent: write(_pipeIn[1], data) ──> Child: read(STDIN_FILENO, buffer)
 *
 * Usage by HTTP method:
 * - GET requests: data is empty (nothing written)
 * - POST requests: data contains form data or JSON payload
 * - PUT requests: data contains uploaded content
 *
 * Important: After this function, _pipeIn[1] must be closed to signal EOF
 * to the child. Without EOF, scripts waiting for more input will hang.
 *
 * Example (PHP):
 *   $input = file_get_contents('php://input');  // Reads until EOF
 *   // If parent doesn't close _pipeIn[1], this blocks forever
 *
 * @param data Request body content (empty for GET, populated for POST/PUT)
 *
 * @note Does nothing if data is empty (optimization for GET requests)
 * @note Uses data.c_str() for direct buffer access (no copy)
 * @warning Parent MUST close _pipeIn[1] after this to signal EOF
 */
void CGIExecutor::writeToChild(const std::string &data) {
  if (!data.empty())
    write(_pipeIn[1], data.c_str(), data.size());
}

/**
 * @brief Reads all output from child process stdout until EOF
 *
 * Captures the complete output from the CGI script by reading from the output
 * pipe in a loop until the child closes its stdout (EOF condition).
 *
 * Buffer size justification (4096 bytes):
 * - 4096 bytes = 4 KB = Standard POSIX page size
 * - Optimal for system I/O operations (used by nginx, apache, kernel)
 * - Matches typical filesystem block size
 * - NOT a magic number - it's a system performance standard
 *
 * Reading algorithm:
 * 1. Allocate 4KB buffer on stack (fast, automatic cleanup)
 * 2. read() blocks until data available or EOF
 * 3. If bytesRead <= 0: EOF or error → break
 * 4. append(buffer, bytesRead) → Only copy actual bytes read (not full 4096)
 * 5. Repeat until EOF
 *
 * Why append(buffer, bytesRead) instead of +=:
 * - buffer is char array, may contain garbage after bytesRead position
 * - operator+= with char* reads until '\0' (undefined behavior with binary
 * data)
 * - append(buffer, count) safely copies exactly count bytes
 *
 * Example output (typical CGI response):
 *   Content-Type: text/html\r\n
 *   \r\n
 *   <!DOCTYPE html>
 *   <html>
 *   <body><h1>Hello World</h1></body>
 *   </html>
 *
 * EOF detection:
 * - Child closes stdout → read() returns 0
 * - Child exits → parent receives SIGCHLD (handled by waitpid)
 *
 * @return Complete output from CGI script (headers + body as single string)
 *
 * @note Blocks until child closes stdout or exits
 * @note Works with both text and binary output (uses append with size)
 * @note 4096 is POSIX standard, not arbitrary "magic number"
 */
std::string CGIExecutor::readChildOutput() {
  std::string result;
  char buffer[4096]; // POSIX page size (optimal for system I/O)

  while (true) {
    ssize_t bytesRead = read(_pipeOut[0], buffer, 4096);

    if (bytesRead <= 0)
      break;                          // EOF (0) or error (-1)
    result.append(buffer, bytesRead); // Append only actual bytes read
  }

  return result;
}
