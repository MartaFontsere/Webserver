27.10.25 - Creación de la estructura inicial del proyecto                                         
31.10.2025 - Creación de la rama Hello World, con un prototipo minimo de webserver que acepta un cliente y responde Hello World   World     
3.11.2025 - Creación de la rama Server multi-client (POLLIN), con un prototipo de webserver que acepta multiples clientes no bloqueantes, preparado para recibir la request en trozos y responde Hello World                


Test:

Terminal 1:
- c++ -Wall -Wextra -Werror -Iincludes src/main.cpp src/Server.cpp src/Client.cpp -o server
- ./server            

Terminal 2:
- python3 script.py
