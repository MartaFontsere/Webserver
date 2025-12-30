#include "../includes/config/ConfigBuilder.hpp"
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "Server.hpp"
#include <csignal>

// podemos cambiar el tipo de variable a volatile sig_atomic_t para que sea más
// seguro:
// sig_atomic_t: Es un tipo de dato que garantiza que se puede leer/escribir en
// una sola operación de CPU (evita problemas si la señal llega justo cuando
// estás leyendo la variable). volatile: Le dice al compilador "no optimices
// esta variable, puede cambiar en cualquier momento fuera de tu control".
// Aunque bool funciona el 99% de las veces, sig_atomic_t es el tipo estándar
// para esto.

// TODO: Valorar si cambiar a sig_atomic_t

bool g_running = true; // Variable global para controlar el bucle principal y
                       // determinar si el servidor debe continuar ejecutándose.

/*
Justificación del uso de una variable global:

Limitación Técnica de signal(): "La función signal() de la librería estándar de
C solo acepta funciones con una firma fija (void (*)(int)). Esto significa que
el manejador de señales no puede recibir un puntero a mi clase Server ni acceder
a variables locales de main."

Necesidad de Comunicación: "Para que el servidor se detenga de forma 'limpia'
(graceful shutdown) y ejecute sus destructores (cerrando sockets y liberando
memoria), necesito que el manejador de señales comunique la orden de parada al
bucle principal de poll. La única forma estándar y segura de compartir este
estado entre un signal handler y el programa principal es mediante una variable
global."

Excepción de Networking: "En proyectos de red como este o IRC, esta es la única
excepción permitida y recomendada para garantizar la gestión limpia de recursos
exigida por el subject."
*/

void signalHandler(int signum) {
  if (signum == SIGINT)
    std::cout << "       \nCtrl+C recibido\n";
  else if (signum == SIGTERM)
    std::cout << "       \nSIGTERM recibido\n";
  g_running = false;
  std::cout << "\n🛑 Signal received, shutting down gracefully..." << std::endl;
}

/*
Manejador de señales para manejar Ctrl+C y otros. Los signal handlers solo
pueden trabajar con cosas muy simples -> Variables globales o estáticas

Esta función se ejecuta automáticamente cuando el proceso recibe una señal del
sistema operativo.

Ejemplos:
Ctrl + C → SIGINT
kill <pid> → SIGTERM

signum es el número de la señal recibida (SIGINT, SIGTERM, etc.).

Por qué se ignora? (void)signum; -> Porque no se usa.
Evita un warning del compilador por parámetro no usado.
  #Ahora lo uso para identificar el tipo de señal recibida e imprimirlo por
terminal

La línea clave es g_running = false; Al cambiar el estado global, le dice al
servidor, sal del bucle principal cuando puedas (es el bucle que mantiene el
servidor funcionando).

No mata el proceso.
No hace exit().
No cierra sockets aquí.

👉 Solo avisa.

Qué pasa cuando haces Ctrl+C
  El SO manda SIGINT
  Se ejecuta signalHandler
  g_running = false
  El while termina
  Sales del loop
  Cierras sockets
  Limpias memoria
  El programa termina ordenadamente

🎯 Shutdown limpio

Por qué signalHandler tiene ese parámetro aunque no lo uses?

  Cuando registras un handler así:
    signal(SIGINT, signalHandler);

  le estás diciendo al sistema operativo:
    “Cuando llegue una señal, llama a esta función”
  La firma está definida por POSIX / C estándar
  No puedes cambiar la firma, aunque no uses el parámetro.
*/

int main(int argc, char **argv) {
  std::string configPath;
  if (argc == 1)
    configPath = "test.conf";
  else
    configPath = argv[1];

  try {
    BlockParser root = parseAndValidateConfig(configPath);
    ConfigBuilder builder;
    std::vector<ServerConfig> servConfigsList =
        builder.buildFromBlockParser(root);

    std::cout << "✅ Configuration loaded: " << servConfigsList.size()
              << " server(s)" << std::endl;

    Server server(servConfigsList);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (!server.init()) {
      return 1;
    }

    server.run();
  } catch (std::exception &e) {
    std::cerr << "❌ Config error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

/* ANTIGUO MAIN:
#include "../includes/config/ConfigBuilder.hpp"
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "Server.hpp"

int main(int argc, char **argv)
{
    std::string configPath;
    if (argc == 1)
        configPath = "test.conf";
    else
        configPath = argv[1];

    Server server("8080");
    try
    {
        BlockParser root = parseAndValidateConfig(configPath);
        ConfigBuilder builder;
        std::vector<ServerConfig> servers = builder.buildFromBlockParser(root);

        std::cout << "✅ Configuration loaded: " << servers.size() << "
server(s)" << std::endl;

        if (!server.init())
        {
            return 1;
        }

        server.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "❌ Config error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
*/