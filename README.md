27.10.25 - Creación de la estructura inicial del proyecto                                         
31.10.2025 - Creación de la rama Hello World, con un prototipo minimo de webserver que acepta un cliente y responde Hello World                                  
3.11.2025 - Creación de la rama Server multi-client (POLLIN), con un prototipo de webserver que acepta multiples clientes no bloqueantes, preparado para recibir la request en trozos y responde Hello World                   
7.11.2025 - Creación de la rama Server multi-client (POLLOUT), con un prototipo de webserver que acepta multiples clientes no bloqueantes, preparado para mandar la respuesta en trozos y responde Hello World                            
13.11.2025 - Creación de la rama Parsing-request, con el mismo prototipo que el anterior pero las responsabilidades divididas entre client y HttpRequest                
14.11.2025 - Creación de la rama Process-and-send-response, con el mismo prototipo que el anterior pero las responsabilidades divididas entre client y HttpResponse       

Test:
 * Terminal 1:                                  
       - c++ -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp src/HttpRequest.cpp src/HttpResponse -o server                     
       - ./server
 * Terminal 2:                                    
       - python3 script.py
