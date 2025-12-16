#include "../../includes/response/HttpResponse.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string &path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        std::cerr << "❌ Error: No se pudo abrir " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

int main()
{
    std::cout << "=== Test HttpResponse ===" << std::endl;

    // Leer HTML
    std::string htmlPath = "/home/pamanzan/Cursus_42/WebServer_oficial/frontend_section/error_files/error404.html";
    std::string htmlContent = readFile(htmlPath);

    if (htmlContent.empty())
    {
        std::cerr << "❌ No se pudo leer el archivo" << std::endl;
        return 1;
    }

    // Crear response
    HttpResponse response;
    response.setStatus(200, "OK");
    response.setContentTypeFromPath(htmlPath);
    response.setBody(htmlContent);

    // Generar HTTP response completa
    std::string httpResponse = response.buildResponse();

    // Mostrar resultado
    std::cout << "\n📤 HTTP Response generada:\n"
              << std::endl;
    std::cout << httpResponse << std::endl;
    std::cout << "\n✅ Test completado" << std::endl;

    return 0;
}