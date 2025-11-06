/**
 * ================================================================
 * ⚙️ CGI TESTING - CONFIGURATION
 * Configuration constants and settings for CGI script testing
 * ================================================================
 */

// Configuration object
const CGI_CONFIG = {
    // Backend endpoint (uncomment and configure for production mode)
    // CGI_ENDPOINT: 'http://localhost:8080', // ⚠️ Configure this for your webserver

    // Default settings
    DEFAULT_SCRIPT: '/cgi-bin/mini.py',
    REQUEST_TIMEOUT: 30000, // 30 seconds
    MAX_HISTORY_ITEMS: 50,
    MAX_OUTPUT_SIZE: 1024 * 1024, // 1MB

    // CGI script endpoints (essential scripts only)
    SCRIPTS: {
        MINI: '/cgi-bin/mini.py',
        ENV_PY: '/cgi-bin/env.py',
        FORM_PY: '/cgi-bin/form.py',
        HELLO_SH: '/cgi-bin/hello.sh',
        HELLO_CGI: '/cgi-bin/hello.cgi'
    },

    // Quick test configurations (active scripts only)
    QUICK_SCRIPTS: [
        { name: '🚀 Mini Test (Python)', script: '/cgi-bin/mini.py', method: 'GET', description: 'Script básico de testing en Python' },
        { name: '🐚 Hello Shell', script: '/cgi-bin/hello.sh', method: 'GET', description: 'Hello world en Bash' },
        { name: '⚡ Hello C++', script: '/cgi-bin/hello.cgi', method: 'GET', description: 'Hello world compilado en C++' },
        { name: '🌍 Environment (Python)', script: '/cgi-bin/env.py', method: 'GET', description: 'Variables de entorno en Python' },
        { name: '📝 Form Handler (Python)', script: '/cgi-bin/form.py', method: 'POST', description: 'Procesador de formularios en Python' }
    ],

    // HTTP methods supported
    HTTP_METHODS: ['GET', 'POST'],

    // Content types for POST data
    CONTENT_TYPES: {
        'application/x-www-form-urlencoded': { name: 'Form Data', default: 'name=value&param=test' },
        'application/json': { name: 'JSON', default: '{\n  "message": "Hello CGI",\n  "number": 42\n}' },
        'text/plain': { name: 'Plain Text', default: 'This is plain text data for the CGI script.' },
        'multipart/form-data': { name: 'Multipart Form', default: 'Use form with file inputs for this type' }
    },

    // Common environment variables for CGI
    COMMON_ENV_VARS: [
        'REQUEST_METHOD',
        'QUERY_STRING',
        'CONTENT_TYPE',
        'CONTENT_LENGTH',
        'SERVER_NAME',
        'SERVER_PORT',
        'SCRIPT_NAME',
        'PATH_INFO',
        'REMOTE_ADDR',
        'HTTP_USER_AGENT'
    ],

    // Status code configurations
    STATUS_CODES: {
        // Success
        200: { name: 'OK', class: 'success', color: '#28a745' },

        // Client Errors
        400: { name: 'Bad Request', class: 'error', color: '#dc3545' },
        403: { name: 'Forbidden', class: 'error', color: '#dc3545' },
        404: { name: 'Not Found', class: 'error', color: '#dc3545' },
        405: { name: 'Method Not Allowed', class: 'error', color: '#dc3545' },

        // Server Errors
        500: { name: 'Internal Server Error', class: 'error', color: '#dc3545' },
        502: { name: 'Bad Gateway', class: 'error', color: '#dc3545' },
        503: { name: 'Service Unavailable', class: 'error', color: '#dc3545' }
    },

    // Output view types
    OUTPUT_VIEWS: {
        OUTPUT: 'output',
        HEADERS: 'headers',
        ERRORS: 'errors',
        ENVIRONMENT: 'environment'
    },

    // UI Messages
    MESSAGES: {
        NO_EXECUTIONS: 'No executions in history',
        NO_OUTPUT: 'No script executed yet',
        LOADING: 'Executing script...',
        SUCCESS: 'Script executed successfully',
        ERROR: 'Script execution failed',
        TIMEOUT: 'Script execution timed out',
        NETWORK_ERROR: 'Network error occurred',
        HISTORY_CLEARED: 'History cleared successfully',
        HISTORY_EXPORTED: 'History exported successfully',
        INVALID_SCRIPT: 'Please enter a valid script path',
        INVALID_PARAMS: 'Invalid parameters format'
    },

    // Local storage keys
    STORAGE_KEYS: {
        HISTORY: 'cgi_test_history',
        SETTINGS: 'cgi_test_settings'
    },

    // Simulation responses for development (essential scripts only)
    SIMULATION_RESPONSES: {
        // Python script - basic testing
        '/cgi-bin/mini.py': {
            output: 'Content-Type: text/plain; charset=utf-8\n\n=== MINI CGI TEST ===\n✅ Script ejecutado correctamente\n\nMétodo: GET\nQuery: test=123\nServidor: localhost\n\nParámetros:\n  test = 123\n\n=== FIN TEST ===\n🎯 CGI funciona perfectamente!',
            headers: {
                'Content-Type': 'text/plain; charset=utf-8',
                'Server': 'webserv/1.0',
                'Date': new Date().toUTCString()
            },
            status: 200,
            execution_time: 45
        },

        // Python script - environment variables
        '/cgi-bin/env.py': {
            output: 'Content-Type: text/plain\n\nREQUEST_METHOD=GET\nSERVER_NAME=localhost\nSERVER_PORT=8080\nSCRIPT_NAME=/cgi-bin/env.py\nQUERY_STRING=test=value\nHTTP_HOST=localhost:8080\nSERVER_SOFTWARE=webserv/1.0',
            headers: {
                'Content-Type': 'text/plain',
                'Server': 'webserv/1.0',
                'Date': new Date().toUTCString()
            },
            status: 200,
            execution_time: 80
        },

        // Python script - form processing
        '/cgi-bin/form.py': {
            output: 'Content-Type: text/html\n\n<!DOCTYPE html>\n<html>\n<head><title>Form Processed</title></head>\n<body>\n<h1>📝 Form Data Processed</h1>\n<p>✅ POST data received!</p>\n</body>\n</html>',
            headers: {
                'Content-Type': 'text/html',
                'Server': 'webserv/1.0',
                'Date': new Date().toUTCString()
            },
            status: 200,
            execution_time: 200
        },

        // Shell script - basic hello
        '/cgi-bin/hello.sh': {
            output: 'Content-Type: text/plain\n\n#!/bin/bash\necho "Content-Type: text/plain"\necho ""\necho "Hello from Bash CGI 🐚🐚🐚"\necho "Script: $SCRIPT_NAME"\necho "Method: $REQUEST_METHOD"\necho "Server: webserv/1.0"\necho "Time: $(date)"\n',
            headers: {
                'Content-Type': 'text/plain',
                'Server': 'webserv/1.0',
                'Date': new Date().toUTCString()
            },
            status: 200,
            execution_time: 85
        },

        // C++ compiled executable
        '/cgi-bin/hello.cgi': {
            output: 'Content-Type: text/plain\n\n/* ⚡ Hello from C++ CGI ⚡*/\n#include <iostream>\n#include <cstdlib>\n\nint main() {\n    std::cout << "Content-Type: text/plain\\n\\n";\n    std::cout << "Hello from C++ CGI executable!\\n";\n    std::cout << "Language: C++\\n";\n    std::cout << "Compiled binary executing...\\n";\n    std::cout << "Server: webserv/1.0\\n";\n    return 0;\n}\n\n✅ Execution successful!',
            headers: {
                'Content-Type': 'text/plain',
                'Server': 'webserv/1.0',
                'Date': new Date().toUTCString()
            },
            status: 200,
            execution_time: 25
        }
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = CGI_CONFIG;
}
