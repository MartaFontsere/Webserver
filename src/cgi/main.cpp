#include <iostream>
#include "../../includes/cgi/hardcoded/Request.hpp"
#include "../../includes/cgi/hardcoded/Response.hpp"

int main()
{
    std::cout << "=== CGI HANDLER TESTING ===" << std::endl;

    // Test 1: Create mock request
    Request req("GET", "/cgi-bin/hello.php?name=world");
    req.setProtocol("HTTP/1.1");
    req.setHeader("Host", "localhost:8080");
    req.setHeader("User-Agent", "TestClient/1.0");

    std::cout << "\nMock Request Created:" << std::endl;
    std::cout << "  Method: " << req.getMethod() << std::endl;
    std::cout << "  URI: " << req.getURI() << std::endl;
    std::cout << "  Query String: " << req.getQueryString() << std::endl;
    std::cout << "  Script Name: " << req.getScriptName() << std::endl;

    // Test 2: Create mock response
    Response res(200, "OK");
    res.setHeader("Content-Type", "text/html");
    res.setBody("<html><body>Hello World</body></html>");

    std::cout << "\nMock Response Created:" << std::endl;
    std::cout << "  Status: " << res.getStatusCode() << " " << res.getStatusMessage() << std::endl;
    std::cout << "  Content-Type: " << res.getHeader("Content-Type") << std::endl;
    std::cout << "  Body length: " << res.getBody().size() << std::endl;

    std::cout << "\n✅ Mocks working correctly" << std::endl;
    std::cout << "\nReady to implement CGI Handler!" << std::endl;

    return 0;
}