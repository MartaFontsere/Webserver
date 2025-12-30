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
- Matar el proceso que esté usando el puerto: kill -9 "PID" (poner el PID obtenido antes sin los " ")
                     

TESTS:

Test multiclient:
 * Terminal 1:                                  
       - c++ -std=c++98 -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
 * Terminal 2:                                    
       - python3 script.py

                                  
Test timeout:
* Terminal 1:                                  
       - c++ -std=c++98 -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
 * Terminal 2:
       - nc localhost 8080


Test Post/Delete:

(opcion 1)
* Terminal 1:                                  
       - c++ -std=c++98 -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
* Terminal 2:                
       - ./test-post-delete.sh

(opcion 2)
* Terminal:                                  
       - c++ -std=c++98 -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
* Navegador:                         
       - localhost:8080/tests/post-delete/test.html

Test Autoindex:

(opcion 1)
* Terminal 1:                                  
       - c++ -std=c++98 -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
* Terminal 2:                
       - ./test-autoindex.sh


(opcion 2)
* Terminal:                                  
       - c++ -std=c++98 -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
* Navegador:                         
       - http://localhost:8080/tests/files/
       - http://localhost:8080/tests/files/	Listado de directorio (autoindex)
       - http://localhost:8080/tests/public/	Página "index.html" (NO autoindex)
       - http://localhost:8080/tests/private/	Error 403 Forbidden
       - http://localhost:8080/tests/files/document1.txt	Contenido del archivo
       - http://localhost:8080/tests/files/subdir/	Listado de subdirectorio
