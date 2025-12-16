#include "../../includes/response/HttpResponse.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// std::string readFile(const std::string &path)
// {
//     std::ifstream file(path.c_str());
//     if (!file.is_open())
//     {
//         std::cerr << "❌ Error: No se pudo abrir " << path << std::endl;
//         return "";
//     }

//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     file.close();

//     return buffer.str();
// }

// int main()
//{
// std::cout << "=== Test HttpResponse ===" << std::endl;

// // Leer HTML
// std::string htmlPath = "/home/pamanzan/Cursus_42/WebServer_oficial/test-autoindex.sh";
// std::string htmlContent = readFile(htmlPath);

// if (htmlContent.empty())
// {
//     std::cerr << "❌ No se pudo leer el archivo" << std::endl;
//     return 1;
// }

// // Crear response
// HttpResponse response;
// response.setStatus(200, "OK");
// response.setContentTypeFromPath(htmlPath);
// response.setBody(htmlContent);

// // Generar HTTP response completa
// std::string httpResponse = response.buildResponse();

// // Mostrar resultado
// std::cout << "\n📤 HTTP Response generada:\n"
//           << std::endl;
// std::cout << httpResponse << std::endl;
// std::cout << "\n✅ Test completado" << std::endl;

//     std::cout << "=== Test Error Pages ===" << std::endl;

//     // Test 1: Error page desde archivo
//     std::map<int, std::string> errorPages;
//     errorPages[404] = "frontend_section/error_files/error404.html";
//     errorPages[500] = "frontend_section/error_files/error500.html";

//     HttpResponse response1;
//     response1.setErrorResponse(404, errorPages);

//     std::cout << "\n📤 Error 404 Response:\n"
//               << response1.buildResponse() << std::endl;

//     // Test 2: Fallback hardcoded (código no en map)
//     HttpResponse response2;
//     response2.setErrorResponse(413, errorPages); // 403 NO está en el map

//     std::cout << "\n📤 Error 413 Fallback:\n"
//               << response2.buildResponse() << std::endl;

//     return 0;
// }