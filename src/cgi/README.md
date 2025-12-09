# CGI Module - WebServer

Complete CGI (Common Gateway Interface) execution module compatible with RFC 3875, implemented in C++98 for the 42-style WebServer project.

## 📋 General Description

This module allows the web server to execute dynamic scripts (PHP, Python, Bash, etc.) using the CGI/1.1 standard. It handles the complete lifecycle of a CGI request: from detection to HTTP response generation.

### Main Features

- ✅ **RFC 3875 compliant** - Complete implementation of the CGI/1.1 standard
- ✅ **Multi-interpreter** - Supports PHP-CGI, Python, Bash and any CGI executable
- ✅ **Process management** - Fork/exec/pipes with robust error handling
- ✅ **Environment variables** - ~20 automatic CGI variables
- ✅ **Complete parsing** - Headers/body separation, status code extraction
- ✅ **No memory leaks** - Manual management with new/delete (verified)
- ✅ **Modular architecture** - 6 independent and testable submodules
- ✅ **Pure C++98** - No external dependencies, no Boost

---

## 🏗️ Architecture

### Processing Pipeline

HTTP Request
↓
┌─────────────────────────────────────────────────┐
│ CGIHandler │ ← Main orchestrator
│ (Single public entry point) │
└─────────────────────────────────────────────────┘
↓
┌──────────────┐ ┌─────────────────┐ ┌───────────────┐
│ CGIDetector │→ │ CGIEnvironment │→ │ CGIExecutor │
└──────────────┘ └─────────────────┘ └───────────────┘
↓ ↓ ↓
┌─────────────────────────────────────────────────────────┐
│ CGIOutputParser │
└─────────────────────────────────────────────────────────┘
↓
HTTP Response

### Modules (6 components)

#### 1. **CGIDetector** (Detection and Resolution)

```cpp
// Detects if a request is CGI
bool isCGIRequest(uri, cgiExts);

// Resolves full script path
std::string resolveScriptPath(uri, root);

// Finds CGI executable for the script
std::string getCGIExecutable(scriptPath, cgiPaths, cgiExts);
```

**Responsibilities:**
- Detection by file extension (.php, .py, .sh)
- Path resolution (handles trailing slashes)
- Extension → executable matching

---

#### 2. **CGIUtils** (Utilities)

```cpp
// Conversions
std::string intToString(int value);
std::string extractQueryString(uri);
std::string toUpperCase(str);

// HTTP headers → CGI transformation
std::map<…> convertHeadersToEnv(headers);
std::string headerToEnvName(headerName);

// execve interface
void stringToCString(source, dest);
```

**Responsibilities:**
- Type conversions (C++98 compatible)
- String processing
- Headers transformation to CGI format

---

#### 3. **CGIEnvironment** (Environment Variables)

```cpp
// Prepare ~20 CGI variables
void prepare(req, scriptPath, scriptName, serverName, port);

// Convert to array for execve
char **toEnvArray() const;

// Free memory
void freeEnvArray(char **env) const;
```

**Responsibilities:**
- Variable preparation according to RFC 3875
- Conversion std::map → char** for execve()
- Memory management (new/delete)

**Prepared variables (20+):**
- `GATEWAY_INTERFACE`, `SERVER_SOFTWARE`, `SERVER_PROTOCOL`
- `SERVER_NAME`, `SERVER_PORT`
- `REQUEST_METHOD`, `QUERY_STRING`
- `SCRIPT_NAME`, `SCRIPT_FILENAME`
- `CONTENT_TYPE`, `CONTENT_LENGTH`
- `HTTP_*` (all HTTP headers converted)
- `REDIRECT_STATUS` (required by php-cgi)

---

#### 4. **CGIExecutor** (Script Execution)

```cpp
// Execute complete CGI script
std::string execute(executable, scriptPath, envp, requestBody);
```

**Responsibilities:**
- Pipe creation (stdin/stdout)
- Process forking
- I/O redirection (dup2)
- CGI interpreter execve
- Output capture
- Termination wait (waitpid)

**Process architecture:**

```
PARENT (webserver)       CHILD (CGI script)
==================       ==================
write(_pipeIn[1])  ──────────>  read(STDIN)
read(_pipeOut[0])  <──────────  write(STDOUT)
```

---

#### 5. **CGIOutputParser** (Output Parsing)

```cpp
// Parse RAW script output
void parse(rawOutput);

// Extract data
int getStatusCode() const;
std::map<…> getHeaders() const;
std::string getBody() const;
```

**Responsibilities:**
- Headers/body separation by `\r\n\r\n`
- Line-by-line header parsing
- Status code extraction (default 200)
- `\r` cleanup (carriage returns)

---

#### 6. **CGIHandler** (Orchestrator)

```cpp
// SINGLE public interface for the webserver
Response handle(const Request &req, const LocationConfig &location);
```

**Responsibilities:**
- Coordination of all modules
- Error handling (404/500)
- Memory cleanup guarantee
- Request → Response conversion

---

## 🔄 Complete Execution Flow

### Example: GET /hello.php?name=world

```cpp
// 1. DETECTION
isCGIRequest("/hello.php?name=world", {".php"})
→ Extension: ".php"
→ Match: true

// 2. PATH RESOLUTION
resolveScriptPath("/hello.php", "./test_scripts")
→ "./test_scripts/hello.php"

getCGIExecutable("./test_scripts/hello.php", {"/usr/bin/php-cgi"}, {".php"})
→ "/usr/bin/php-cgi"

// 3. ENVIRONMENT PREPARATION
env.prepare(…)
→ REQUEST_METHOD=GET
→ QUERY_STRING=name=world
→ SCRIPT_NAME=/hello.php
→ SCRIPT_FILENAME=./test_scripts/hello.php
→ HTTP_HOST=localhost:8080
→ … (20 total variables)

envp = env.toEnvArray()
→ ["REQUEST_METHOD=GET", "QUERY_STRING=name=world", …, NULL]

// 4. EXECUTION
executor.execute("/usr/bin/php-cgi", "./test_scripts/hello.php", envp, "")
→ fork()
→ Child: execve("/usr/bin/php-cgi", argv, envp)
→ Parent: read(STDOUT) → capture output
→ waitpid() → wait for termination

Captured output:
"Content-Type: text/html\r\n\r\n

Hello world
"
// 5. PARSING
parser.parse(output)
→ _headers = {"Content-Type": "text/html"}
→ _body = "

Hello world
"
→ _statusCode = 200 (default, no Status header)
// 6. RESPONSE
Response(200, "

Hello world
")
→ Sent to client
```

---

## 📦 WebServer Integration

### Public interface (CGIHandler only)

```cpp
#include "cgi/CGIHandler.hpp"

// In your server:
CGIHandler cgiHandler;
Response response = cgiHandler.handle(request, locationConfig);
// Done! response contains the HTML/JSON/etc.
```

### LocationConfig Requirements

```cpp
struct LocationConfig {
std::string root; // "./test_scripts"
std::vector<std::string> cgiPaths; // {"/usr/bin/php-cgi", "/usr/bin/python3"}
std::vector<std::string> cgiExts; // {".php", ".py"}
std::string serverName; // "localhost"
int serverPort; // 8080
};
```

### Building from ConfigParser

```cpp
LocationConfig buildLocationConfig(const BlockParser &locationBlock)
{
LocationConfig loc;

for (directive in locationBlock.directives) {
    if (directive.name == "cgi_ext")
        loc.cgiExts = directive.values;
    if (directive.name == "cgi_path")
        loc.cgiPaths = directive.values;
    if (directive.name == "root")
        loc.root = directive.values[0];
}

// serverName and serverPort come from parent server block

return loc;
}
```

### Configuration example (nginx-like)

```nginx
server {
listen 8080;
server_name localhost;
root ./test_scripts;

location ~ \.php$ {
    allow_methods GET POST;
    cgi_path /usr/bin/php-cgi;
    cgi_ext .php .php5 .phtml;
}

location ~ \.py$ {
    allow_methods GET POST;
    cgi_path /usr/bin/python3;
    cgi_ext .py;
}
}
```

---

## 🧪 Testing

### Implemented tests (30+)

```bash
cd src/cgi
g++ -Wall -Wextra -Werror -std=c++98 \
hardcoded/*.cpp \
*.cpp \
-I../../includes/cgi \
-o test

./test
```

**Expected output:**

```
✅ CGIDetector Tests (10/10)
✅ CGIUtils Tests (5/5)
✅ CGIEnvironment Tests (5/5)
✅ CGIExecutor Tests (3/3)
✅ CGIOutputParser Tests (4/4)
✅ CGIHandler End-to-End (3/3)

Total: 30/30 PASSED
```

### Included test scripts

```bash
test_scripts/
├── hello.php # PHP with query params
├── echo.py # Python with CGI variables
├── env.sh # Bash that prints environment
└── form.html # Test form
```

---

## 🔍 Technical Decisions

### 1. No `chdir()` to script directory
**Decision:** Use full path in `argv[1]`

**Reasons:**
- Universal for PHP, Python, Bash, any interpreter
- Simpler (no path parsing)
- Safer (script cannot escape directory)
- RFC 3875 does not require chdir()

### 2. 4096 byte buffer
**Decision:** Fixed 4KB size for `read()`

**Reasons:**
- 4096 = standard POSIX page size
- Optimal for I/O operations on UNIX/Linux
- Used by nginx, apache, kernel
- NOT a magic number, it's a system standard

### 3. `new`/`delete` instead of `malloc`/`free`
**Decision:** Pure C++ memory management

**Reasons:**
- Consistency with C++98
- Type-safety
- RAII compatible (though not applied here due to execve interface)

### 4. `std::stringstream` instead of `itoa`/`sprintf`
**Decision:** Conversions with C++ streams

**Reasons:**
- `itoa()` is not C++98 standard
- `sprintf()` is C-style (buffer overflow risk)
- `std::stringstream` safe and standard

### 5. `REDIRECT_STATUS=200`
**Decision:** Additional variable not in RFC 3875

**Reasons:**
- PHP-CGI requires it for security
- Without it: "Security Alert! PHP CGI cannot be accessed directly"
- Common in real web servers

---

## 📚 References

### Implemented standards

- **RFC 3875** - The Common Gateway Interface (CGI) Version 1.1
  - Section 4.1: Request Meta-Variables
  - Section 6: CGI Response
  - Section 6.3.3: Status header

- **POSIX.1-2001**
  - `fork()`, `execve()`, `pipe()`, `dup2()`, `waitpid()`
  - File descriptor management
  - Process creation and IPC

- **C++98 Standard**
  - `std::string`, `std::vector`, `std::map`
  - `std::stringstream`, `std::istringstream`
  - Iterators (`const_iterator`)
  - Exceptions (`std::runtime_error`)

### Inspiration

- NGINX - CGI module implementation
- Apache - mod_cgi architecture
- RFC examples and best practices

---

## 📂 File Structure

```
cgi/
├── includes/cgi/
│ ├── CGIDetector.hpp
│ ├── CGIEnvironment.hpp
│ ├── CGIExecutor.hpp
│ ├── CGIHandler.hpp
│ ├── CGIOutputParser.hpp
│ └── CGIUtils.hpp
│
├── src/cgi/
│ ├── CGIDetector.cpp [Documented with Doxygen]
│ ├── CGIEnvironment.cpp [Documented with Doxygen]
│ ├── CGIExecutor.cpp [Documented with Doxygen]
│ ├── CGIHandler.cpp [Documented with Doxygen]
│ ├── CGIOutputParser.cpp [Documented with Doxygen]
│ ├── CGIUtils.cpp [Documented with Doxygen]
│ └── main.cpp [Testing suite]
│
├── test_scripts/
│ ├── hello.php
│ ├── echo.py
│ ├── env.sh
│ └── form.html
│
└── test.conf [Test configuration]
```

---

## ✅ Project Status

### Completeness

| Component           | Status | Tests |
|----------------------|--------|-------|
| CGIDetector          | ✅ 100% | 10/10 |
| CGIUtils             | ✅ 100% | 5/5   |
| CGIEnvironment       | ✅ 100% | 5/5   |
| CGIExecutor          | ✅ 100% | 3/3   |
| CGIOutputParser      | ✅ 100% | 4/4   |
| CGIHandler           | ✅ 100% | 3/3   |
| **TOTAL**            | ✅ 100% | 30/30 |
| **Documentation**    | ✅ 100% | Complete Doxygen |

### Real verification

- ✅ PHP-CGI executes `.php` scripts correctly
- ✅ Python3 executes `.py` scripts correctly
- ✅ CGI variables accessible from scripts
- ✅ Query strings processed
- ✅ POST data sent to stdin
- ✅ Complete output captured (headers + body)
- ✅ No memory leaks (valgrind clean)
- ✅ No compilation warnings
- ✅ Compilation `-Wall -Wextra -Werror -std=c++98`

---

## 🚀 Next Steps (Integration)

### 1. Connect Config Parser → LocationConfig

```cpp
LocationConfig ServerConfig::getLocation(const std::string &uri) const
{
// Find location block matching uri
// Build LocationConfig from BlockParser
// Include serverName/serverPort from parent server block
}
```

### 2. Integrate into Request Router

```cpp
Response Server::handleRequest(const Request &req)
{
LocationConfig location = _config.getLocation(req.getURI());
if (CGIDetector::isCGIRequest(req.getURI(), location.cgiExts))
{
    CGIHandler cgiHandler;
    return cgiHandler.handle(req, location);
}

// Other handlers (static files, proxy, etc.)
}
```

### 3. Custom Error Pages

```cpp
if (response.getStatusCode() >= 400 &&
location.errorPages.count(response.getStatusCode()))
{
std::string html = readFile(location.errorPages[response.getStatusCode()]);
response.setBody(html);
}
```

### 4. Timeout Protection (optional)

```cpp
alarm(30); // 30 second maximum
waitpid(_childPid, &status, 0);
alarm(0); // Cancel alarm

if (timeout)
kill(_childPid, SIGKILL);
```

---

## 👥 Authors

**PatriPomes**
**@pamanzana**

---

## 📝 License

Educational project - 42 Network

---

## 🙏 Acknowledgments

- WebServ Subject (42 Network)
- RFC 3875 authors
- NGINX/Apache developers (architectural inspiration)

---

**Last updated:** 2024-12-09  
**Module version:** 1.0 - PRODUCTION READY  
**Status:** ✅ Complete and functional