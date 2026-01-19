27.10.25 - Creación de la estructura inicial del proyecto                                         
31.10.2025 - Creación de la rama Hello World, con un prototipo minimo de webserver que acepta un cliente y responde Hello World                                  
3.11.2025 - Creación de la rama Server multi-client (POLLIN), con un prototipo de webserver que acepta multiples clientes no bloqueantes, preparado para recibir la request en trozos y responde Hello World                   
7.11.2025 - Creación de la rama Server multi-client (POLLOUT), con un prototipo de webserver que acepta multiples clientes no bloqueantes, preparado para mandar la respuesta en trozos y responde Hello World                            
13.11.2025 - Creación de la rama Parsing-request, con el mismo prototipo que el anterior pero las responsabilidades divididas entre client y HttpRequest                
14.11.2025 - Creación de la rama Process-and-send-response, con el mismo prototipo que el anterior pero las responsabilidades divididas entre client y HttpResponse       
14.11.2025 - Creación de la rama Server-engine para centrar el desarrollo de la lógica interna del servidor

IMPORTANTE: si al ejecutar el servidor aparece este mensaje: 
 - Error en bind(): Address already in use
 - ❌ Error: no se pudo crear el socket.
                 
Quiere decir que se ha quedado el socket abierto

Para gestionarlo:

- Listarlos (verás el PID): lsof -i :8080
- Matar el proceso que esté usando el puerto: kill -9 "PID"

O directamente: pkill webServer.out

PERMISOS:
🔒 Pruebas de Error 403

Para probar el error 403 Forbidden con un archivo sin permisos de lectura:

Quitar permisos (antes de probar):

- chmod 000 www/tests/files/secret.pdf

Restaurar permisos (antes de hacer commit):

- chmod 644 www/tests/files/secret.pdf

⚠️ Nota: Git no puede leer archivos con permisos 000. Restaura los permisos antes de hacer commit.



---------- TESTS -------------

PRUEBAS BÁSICAS Y DE NAVEGACIÓN:

Servir archivo index.html (página de bienvenida):
 * Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
 * Navegador:                                    
       - http://localhost:8080/

Test Autoindex:

(opción 1)
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Terminal 2:                
       - ./tests/scripts/test-autoindex.sh

(opción 2)
* Terminal:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Navegador:                         
       - http://localhost:8080/tests/files/	Listado del contenido del directorio (autoindex (listado de archivos) generado automáticamente por el servidor)     
       - http://localhost:8080/tests/public/	Página "index.html" (NO autoindex, aunque está activado tiene prioridad el archivo index.html si existe)       
       - http://localhost:8080/tests/private/	Error 403 Forbidden (si una carpeta no tiene index.html y el autoindex está OFF, el servidor protege la carpeta y no deja cotillear el contenido)        
       - http://localhost:8080/tests/files/document1.txt	Contenido del archivo     
       - http://localhost:8080/tests/files/subdir/	Listado de subdirectorio
         
  
PRUEBAS DE MÉTODOS HTTP (POST/DELETE)

Test Post/Delete:

(opción 1)
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Terminal 2:                
       - ./tests/scripts/test-post-delete.sh

(opción 2)
* Terminal:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Navegador:                         
       - localhost:8080/post-delete/test.html Probar los botones POST (sube un archivo a /uploads) y DELETE (crea un archivo temporal y luego lo borras introduciendo su nombre)


PRUEBAS DE CGI Y REDIRECCIONES

Test CGI:

(opción 1)
* Terminal 1:                                  
       - make                    
       - ./webServer.out tests/configs/default.conf
* Terminal 2:                          
       - ./tests/scripts/test_cgi.sh      
Verifica GET/POST con Python, ejecución de scripts Bash (.sh), manejo de errores 500 y headers personalizados.

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

