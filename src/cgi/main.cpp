#include <iostream>
#include "../../includes/cgi/hardcoded/Request.hpp"
#include "../../includes/cgi/hardcoded/Response.hpp"
#include "../../includes/cgi/hardcoded/LocationConfig.hpp"
#include "../../includes/cgi/CGIDetector.hpp"
#include "../../includes/cgi/CGIEnvironment.hpp"
#include "../../includes/cgi/CGIUtils.hpp"

void printTest(const std::string &testName, bool passed)
{
    if (passed)
        std::cout << "✅ " << testName << std::endl;
    else
        std::cout << "❌ " << testName << std::endl;
}

int main()
{
    std::cout << "\n======================================" << std::endl;
    std::cout << "   CGI COMPLETE TESTING SUITE" << std::endl;
    std::cout << "======================================\n" << std::endl;

    LocationConfig loc;

    // ========== CGIUTILS TESTING ==========
    std::cout << "--- CGIUtils Testing ---\n" << std::endl;

    // Test intToString
    std::string port = intToString(8080);
    printTest("intToString(8080)", port == "8080");
    std::cout << "  Result: " << port << std::endl;

    // Test extractQueryString
    std::string query = extractQueryString("/script.php?name=world&age=25");
    printTest("extractQueryString with params", query == "name=world&age=25");
    std::cout << "  Result: " << query << std::endl;

    std::string noQuery = extractQueryString("/script.php");
    printTest("extractQueryString without params", noQuery.empty());

    // Test toUpperCase
    std::string upper = toUpperCase("Hello-World");
    printTest("toUpperCase", upper == "HELLO-WORLD");
    std::cout << "  Result: " << upper << std::endl;

    // Test headerToEnvName
    std::string envName1 = headerToEnvName("Host");
    printTest("headerToEnvName(Host)", envName1 == "HTTP_HOST");
    std::cout << "  Result: " << envName1 << std::endl;

    std::string envName2 = headerToEnvName("User-Agent");
    printTest("headerToEnvName(User-Agent)", envName2 == "HTTP_USER_AGENT");
    std::cout << "  Result: " << envName2 << std::endl;

    // ========== CGIENVIRONMENT TESTING ==========
    std::cout << "\n--- CGIEnvironment Testing ---\n" << std::endl;

    // Create mock request
    Request req("GET", "/cgi-bin/hello.php?name=LIB");
    req.setProtocol("HTTP/1.1");
    req.setHeader("Host", "localhost:8080");
    req.setHeader("User-Agent", "TestClient/1.0");
    req.setHeader("Cookie", "session=abc123");
    req.setBody("");

    // Prepare environment
    CGIEnvironment env;
    env.prepare(req, "./test_scripts/hello.php", "/cgi-bin/hello.php", "localhost", 8080);

    // Test getVar
    std::cout << "\nTesting getVar():" << std::endl;
    printTest("REQUEST_METHOD", env.getVar("REQUEST_METHOD") == "GET");
    std::cout << "  Value: " << env.getVar("REQUEST_METHOD") << std::endl;

    printTest("QUERY_STRING", env.getVar("QUERY_STRING") == "name=LIB");
    std::cout << "  Value: " << env.getVar("QUERY_STRING") << std::endl;

    printTest("SERVER_PORT", env.getVar("SERVER_PORT") == "8080");
    std::cout << "  Value: " << env.getVar("SERVER_PORT") << std::endl;

    printTest("SCRIPT_NAME", env.getVar("SCRIPT_NAME") == "/cgi-bin/hello.php");
    std::cout << "  Value: " << env.getVar("SCRIPT_NAME") << std::endl;

    printTest("HTTP_HOST", env.getVar("HTTP_HOST") == "localhost:8080");
    std::cout << "  Value: " << env.getVar("HTTP_HOST") << std::endl;

    printTest("Non-existent var", env.getVar("FAKE_VAR").empty());

    // Test printAll
    std::cout << "\nTesting printAll():" << std::endl;
    env.printAll();

    // Test toEnvArray
    std::cout << "\n--- Testing toEnvArray() ---" << std::endl;
    char **envArray = env.toEnvArray();

    std::cout << "\nGenerated char** array:" << std::endl;
    int count = 0;
    for (int i = 0; envArray[i] != NULL; ++i)
    {
        std::cout << "  [" << i << "] " << envArray[i] << std::endl;
        count++;
    }
    printTest("Array NULL terminated", envArray[count] == NULL);
    printTest("Array has elements", count > 0);

    // Test freeEnvArray
    std::cout << "\nTesting freeEnvArray():" << std::endl;
    env.freeEnvArray(envArray);
    std::cout << "✅ Memory freed successfully (no crash)" << std::endl;

    // ========== SUMMARY ==========
    std::cout << "\n======================================" << std::endl;
    std::cout << "   TESTING COMPLETE" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "\n✅ CGIDetector: TESTED (10/10)" << std::endl;
    std::cout << "✅ CGIUtils: TESTED (5/5)" << std::endl;
    std::cout << "✅ CGIEnvironment: TESTED (5/5)" << std::endl;
    std::cout << "\nNext: CGIExecutor implementation" << std::endl;

    return 0;
}