# 🧪 Guía Completa de Pruebas (Webserver 42)

Esta guía detalla cómo verificar todas las funcionalidades del servidor, tanto mediante scripts automáticos como de forma manual en el navegador.

> [!IMPORTANT]
> Para todas las pruebas, se recomienda usar la configuración completa:
> `make && ./webServer.out tests/configs/mega_test.conf`

---

## 1. Pruebas Básicas y de Navegación

### Servir archivo index.html
*   **Terminal**: `./webServer.out tests/configs/mega_test.conf`
*   **Navegador**: [http://localhost:8080/](http://localhost:8080/)
*   **Resultado**: Debería mostrar la página de bienvenida "Hola desde mi webserv!".

### Autoindex (Listado de archivos)
*   **Navegador**: [http://localhost:8080/tests/files/](http://localhost:8080/tests/files/)
*   **Resultado**: Listado generado automáticamente. Prueba a entrar en subdirectorios o archivos con espacios.

### Prioridad Index vs Autoindex
*   **Navegador**: [http://localhost:8080/tests/public/](http://localhost:8080/tests/public/)
*   **Resultado**: Muestra `index.html` aunque el autoindex esté ON.

### Error 403 Forbidden
*   **Navegador**: [http://localhost:8080/tests/private/](http://localhost:8080/tests/private/)
*   **Resultado**: Error 403 (Autoindex OFF y sin index.html).

---

## 2. Pruebas de Métodos HTTP (POST/DELETE)

### Opción A: Script Automático
*   **Terminal**: `./tests/scripts/test-post-delete.sh`
*   **Resultado**: Crea un archivo, lo borra y verifica que los uploads se mantienen.

### Opción B: Panel de Control (Navegador)
*   **Navegador**: [http://localhost:8080/tests/post-delete/test.html](http://localhost:8080/tests/post-delete/test.html)
*   **Acciones**:
    1.  **POST**: Sube un archivo a `/uploads`.
    2.  **DELETE**: Crea un archivo temporal y luego bórralo introduciendo su nombre.

---

## 3. CGI (Scripts Dinámicos)

### Opción A: Script Automático (Unificado)
*   **Terminal**: `./tests/scripts/test_cgi.sh`
*   **Resultado**: Verifica GET/POST con Python, ejecución de scripts Bash (.sh), manejo de errores 500 y headers personalizados.

### Opción B: Navegador
*   **Python (GET)**: [http://localhost:8080/cgi-bin/test.py?user=Marta](http://localhost:8080/cgi-bin/test.py?user=Marta)
*   **Bash (GET)**: [http://localhost:8080/cgi-bin/hello.sh](http://localhost:8080/cgi-bin/hello.sh)
*   **Headers (GET)**: [http://localhost:8080/cgi-bin/header.py](http://localhost:8080/cgi-bin/header.py)
*   **Error 500 (GET)**: [http://localhost:8080/cgi-bin/error.py](http://localhost:8080/cgi-bin/error.py)
*   **PHP (GET)**: [http://localhost:8080/cgi-bin/hello.php](http://localhost:8080/cgi-bin/hello.php) (Dará 500 si no tienes PHP instalado).

### Redirecciones (301)
*   **Navegador**: [http://localhost:8080/google](http://localhost:8080/google)
*   **Resultado**: Salto automático a google.com.

---

## 4. Errores y Límites

### Páginas de Error Personalizadas
*   **Navegador**: [http://localhost:8080/ruta-inexistente](http://localhost:8080/ruta-inexistente)
*   **Resultado**: Muestra tu archivo `custom_404.html` ("Lo siento chati...").

### Límites de Body (413 Payload Too Large)
*   **Terminal**: `./tests/scripts/test_limits.sh`
*   **Resultado**: Verifica que peticiones mayores de 100 bytes (en esta config) son rechazadas.

---

## 5. Pruebas Avanzadas

### Multicliente y Stress Test
*   **Terminal**: `python3 tests/scripts/number_clients_stress_test.py`
*   **Resultado**: Lanza 20 clientes concurrentes para verificar que el servidor no se bloquea.

### Timeout (nc)
*   **Terminal**: `nc -v localhost 8080` (y esperar sin escribir nada).
*   **Resultado**: El servidor debería cerrar la conexión tras el tiempo de inactividad configurado.

### Virtual Hosts
*   **Terminal**: `./tests/scripts/test_vhosts.sh`
*   **Resultado**: Verifica que el servidor responde distinto según el header `Host: marta.com`.

### Alias
*   **Terminal**: `./tests/scripts/test_alias.sh`
*   **Resultado**: Verifica que `/test-alias/` sirve archivos de una carpeta distinta a la raíz (`tests/test_assets`).

### Múltiples Puertos
*   **Terminal**: `./tests/scripts/test_ports.sh`
*   **Resultado**: Verifica que el servidor escucha en el 8080 y en el 9999 simultáneamente.

---

## 6. Gestión del Servidor (Cierre Limpio)
*   **Acción**: Pulsa `Ctrl+C` en la terminal del servidor.
*   **Resultado**: Verás el mensaje `🛑 Signal received, shutting down gracefully...`. El puerto 8080 se liberará inmediatamente.
