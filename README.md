# WebServer - Nginx-Style HTTP Server

A high-performance HTTP/1.1 web server implementation inspired by NGINX, built from scratch in C++98. This project implements a fully functional web server with support for multiple virtual hosts, CGI execution, file uploads, and modern HTTP features.

## 🚀 Features

### Core HTTP Features
- **HTTP/1.1 Protocol** - Full implementation with persistent connections
- **Multiple HTTP Methods** - GET, POST, DELETE, HEAD support
- **Non-blocking I/O** - Event-driven architecture using `poll()`
- **Chunked Transfer Encoding** - Support for streaming large requests/responses
- **Virtual Hosts** - Multiple server blocks with different configurations
- **Custom Error Pages** - Configurable error pages per status code

### Advanced Capabilities
- **CGI Support** - Execute Python, Bash, and other CGI scripts
- **File Uploads** - Handle multipart/form-data with size limits
- **Directory Listing** - Auto-index functionality for directories
- **HTTP Redirects** - 301/302 redirects with customizable rules
- **Static File Serving** - Efficient serving of HTML, CSS, JS, images
- **Request/Response Chunking** - Handle large payloads efficiently

### Configuration System
- **NGINX-style Config** - Familiar configuration file syntax
- **Syntax Validation** - Parse-time validation of config structure
- **Semantic Validation** - Runtime validation of config values
- **Flexible Routing** - Location blocks with pattern matching
- **Multiple Servers** - Run multiple servers on different ports

## 📋 Requirements

- **Compiler**: g++ or clang++ with C++98 support
- **OS**: Linux or macOS
- **Build Tool**: GNU Make
- **Optional**: Python 3.x (for CGI tests)

## 🔧 Installation

### Quick Start

```bash
# Clone the repository
git clone https://github.com/MartaFontsere/Webserver.git
cd WebServer_oficial

# Build the project
make

# Run with default configuration
./webServer.out

# Run with custom configuration
./webServer.out tests/configs/default.conf
```

### Build Targets

```bash
make          # Compile the server
make clean    # Remove object files
make fclean   # Remove object files and executable
make re       # Recompile everything
```

## 🎯 Usage

### Basic Usage

```bash
# Start server with default config
./webServer.out

# Start server with custom config file
./webServer.out path/to/config.conf

# Graceful shutdown
Ctrl+C  # Sends SIGINT for clean shutdown
```

### Configuration File Example

```nginx
server {
    listen 8080;
    host 127.0.0.1;
    server_name localhost;
    root ./www;
    index index.html index.php;
    client_max_body_size 1048576;
    error_page 404 /404.html;
    error_page 500 /500.html;

    location / {
        allow_methods GET POST DELETE HEAD;
        root ./www;
        index index.html;
        autoindex on;
    }

    location /cgi-bin {
        allow_methods GET POST;
        cgi_path /usr/bin/python3;
        cgi_ext .py;
    }

    location /uploads {
        allow_methods GET POST DELETE;
        upload_path ./www/uploads;
        client_max_body_size 5242880;
    }
}
```

## 🧪 Testing

### Quick Tests

```bash
# Run automated test suite
./tests/scripts/test-autoindex.sh
./tests/scripts/test-post-delete.sh
./tests/scripts/test_cgi.sh

# Test with web browser
# Start server: ./webServer.out tests/configs/default.conf
# Open browser: http://localhost:8080/
```

### Manual Testing

#### Static File Serving
```bash
# Serve welcome page
http://localhost:8080/

# Serve specific file
http://localhost:8080/tests/files/document1.txt
```

#### Directory Listing (Autoindex)
```bash
# Auto-generated directory listing
http://localhost:8080/tests/files/

# Subdirectory listing
http://localhost:8080/tests/files/subdir/

# Index file takes priority (no autoindex)
http://localhost:8080/tests/public/
```

#### Error Handling
```bash
# 403 Forbidden (directory without index and autoindex off)
http://localhost:8080/tests/private/

# 404 Not Found
http://localhost:8080/nonexistent.html
```

#### File Upload (POST)
```bash
# Upload interface
http://localhost:8080/post-delete/test.html

# Or use curl
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads/
```

#### File Deletion (DELETE)
```bash
# Delete file via curl
curl -X DELETE http://localhost:8080/uploads/test.txt

# Or use web interface
http://localhost:8080/post-delete/test.html
```

#### CGI Execution
```bash
# Python CGI script
http://localhost:8080/cgi-bin/test.py

# Bash CGI script
http://localhost:8080/cgi-bin/test.sh
```

## 📁 Project Structure

```
WebServer_oficial/
├── main.cpp                    # Entry point and signal handling
├── Makefile                    # Build configuration (auto-detects sources)
├── includes/                   # Header files
│   ├── config/                 # Configuration classes
│   ├── config_parser/          # Config file parser
│   ├── core/                   # Server core
│   ├── http/                   # HTTP protocol handling
│   ├── network/                # Network I/O
│   └── cgi/                    # CGI execution
├── src/                        # Source files
│   ├── config/                 # Configuration implementation
│   ├── config_parser/          # Parser implementation
│   ├── core/                   # Server implementation
│   ├── http/                   # HTTP implementation
│   ├── network/                # Network implementation
│   └── cgi/                    # CGI implementation
├── tests/                      # Test suite
│   ├── configs/                # Test configuration files
│   └── scripts/                # Automated test scripts
├── www/                        # Document root (static files)
└── README.md                   # This file
```

## 🛠️ Troubleshooting

### Port Already in Use

If you see this error:
```
Error en bind(): Address already in use
❌ Error: no se pudo crear el socket.
```

**Solution:**
```bash
# Find process using the port
lsof -i :8080

# Kill the process
kill -9 <PID>

# Or kill all instances
pkill webServer.out
```

### Permission Issues (403 Forbidden)

Testing 403 errors with restricted files:

```bash
# Remove all permissions (for testing)
chmod 000 www/tests/files/secret.pdf

# Restore permissions (before commit)
chmod 644 www/tests/files/secret.pdf
```

⚠️ **Note**: Git cannot track files with `000` permissions. Always restore permissions before committing.

### CGI Scripts Not Executing

Ensure CGI scripts have execution permissions:
```bash
chmod +x www/cgi-bin/script.py
chmod +x www/cgi-bin/script.sh
```

## 🏗️ Architecture

### Modular Design

The server is organized into independent modules:

- **config_parser**: Parses and validates configuration files
- **config**: Converts parsed config to typed structures
- **network**: Socket management and I/O multiplexing
- **http**: HTTP protocol parsing and response generation
- **core**: Server engine and request routing
- **cgi**: CGI script execution and environment setup

### Request Flow

```
Client → Network → HTTP Parser → Router → Handler → HTTP Response → Network → Client
                                    ↓
                              CGI / Static / Upload
```

### Key Technologies

- **I/O Multiplexing**: `poll()` for non-blocking event-driven I/O
- **Memory Safety**: RAII principles, STL containers (no raw pointers)
- **C++98 Standard**: Full compatibility with older compilers
- **Signal Handling**: Graceful shutdown on SIGINT/SIGTERM

## 📚 Documentation

Detailed documentation available in module-specific README files:

- [Configuration Parser](src/config_parser/README.md) - Config file parsing
- [Configuration Builder](src/config/README.md) - Config object construction
- More documentation coming soon...

## 🤝 Contributing

This is a student project for 42 School. While external contributions are not accepted, feel free to fork and learn from the code.

## 👥 Team

- **Pablo Manzan** - [@pamanzan](https://github.com/pamanzan)
- **Marta Fontsere** - [@MartaFontsere](https://github.com/MartaFontsere)  
- **Lucas Moyano** - [@LucasMoyano](https://github.com/LucasMoyano)

## 📄 License

This project is part of the 42 School curriculum and follows their academic guidelines.

## 🎓 Project Information

**School**: 42 Barcelona  
**Project**: Webserv  
**Language**: C++98  
**Started**: October 27, 2025

(opción 2)
* Terminal:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Navegador:                         
       - Python (GET): http://localhost:8080/cgi-bin/test.py?user=Marta Para ver que el servidor es capaz de ejecutar scripts de Python, pasarles variables y devolver el resultado en tiempo real. En este caso procesa la variable user y devuelve el HTML dinámico        
       - Bash (GET): http://localhost:8080/cgi-bin/hello.sh      
       - Headers (GET): http://localhost:8080/cgi-bin/header.py     
       - Error 500 (GET): http://localhost:8080/cgi-bin/error.py       
       - PHP (GET): http://localhost:8080/cgi-bin/hello.php Dará 500 si no tienes PHP instalado     


Redirecciones:
* Terminal:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Navegador:                         
       - http://localhost:8080/google El navegador debería saltar automáticamente a google.com (estamos probando el comando return 301 de la configuración)


PRUEBAS DE ERRORES Y LÍMITES

Página de Error Personalizada:
* Terminal:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Navegador:                         
       - http://localhost:8080/lo-que-sea-que-no-exista Manejo de errores 404 con archivo HTML propio (muestra la página personalizada)

Límites de Body (413 Payload Too Large)
* Terminal:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Terminal:                         
       - /tests/scripts/test_limits.sh            
  Verifica que peticiones mayores de 100 bytes (en esta config) son rechazadas

PRUEBAS AVANZADAS:

Test multiclient:
 * Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
 * Terminal 2:                                    
       - python3 tests/scripts/number_clients_stress_test.py          
Lanza 20 clientes concurrentes para verificar que el servidor no se bloquea.

                                  
Test timeout (nc):
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
 * Terminal 2:
       - nc - v localhost 8080 (y esperar sin escribir nada)
El servidor debería cerrar la conexión tras el tiempo de inactividad configurado

Test Virtual Host:
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
 * Terminal 2:
       - ./tests/scripts/test_vhosts.sh       
Verifica que el servidor responde distinto según el header `Host: marta.com`.

Test Alias:
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
 * Terminal 2:
       - ./tests/scripts/test_alias.s
Verifica que `/test-alias/` sirve archivos de una carpeta distinta a la raíz (`tests/test_assets`)

Test Múltiples puertos:
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
 * Terminal 2:
       - ./tests/scripts/test_ports.sh    
Verifica que el servidor escucha en el 8080 y en el 9999 simultáneamente

PRUEBAS DE GESTIÓN DEL SERVIDOR          
         
Test Cierre limpio del servidor:
* Terminal 1:                                  
       - make                     
       - ./webServer.out tests/configs/default.conf     
       - Pulsar `Ctrl+C` en la terminal del servidor     
Verás el mensaje `🛑 Signal received, shutting down gracefully...`. El puerto 8080 se liberará inmediatamente.

