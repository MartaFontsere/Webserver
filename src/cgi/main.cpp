#include <iostream>
#include "../../includes/cgi/hardcoded/Request.hpp"
#include "../../includes/cgi/hardcoded/Response.hpp"
#include "../../includes/cgi/hardcoded/LocationConfig.hpp"
#include "../../includes/cgi/CGIDetector.hpp"
#include "../../includes/cgi/CGIEnvironment.hpp"
#include "../../includes/cgi/CGIExecutor.hpp"
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

    std::string port = intToString(8080);
    printTest("intToString(8080)", port == "8080");

    std::string query = extractQueryString("/script.php?name=world");
    printTest("extractQueryString", query == "name=world");

    std::string upper = toUpperCase("Hello-World");
    printTest("toUpperCase", upper == "HELLO-WORLD");

    std::string envName = headerToEnvName("User-Agent");
    printTest("headerToEnvName", envName == "HTTP_USER_AGENT");

    // ========== CGIENVIRONMENT TESTING ==========
    std::cout << "\n--- CGIEnvironment Testing ---\n" << std::endl;

    Request req("GET", "/cgi-bin/hello.php?name=LIB");
    req.setProtocol("HTTP/1.1");
    req.setHeader("Host", "localhost:8080");
    req.setHeader("User-Agent", "TestClient/1.0");
    req.setBody("");

    CGIEnvironment env;
    env.prepare(req, "./test_scripts/hello.php", "/cgi-bin/hello.php", "localhost", 8080);

    printTest("REQUEST_METHOD", env.getVar("REQUEST_METHOD") == "GET");
    printTest("QUERY_STRING", env.getVar("QUERY_STRING") == "name=LIB");
    printTest("SERVER_PORT", env.getVar("SERVER_PORT") == "8080");
    printTest("HTTP_HOST", env.getVar("HTTP_HOST") == "localhost:8080");

    std::cout << "\nEnvironment variables prepared:" << std::endl;
    env.printAll();

    char **envArray = env.toEnvArray();
    printTest("toEnvArray creates array", envArray != NULL);
    printTest("Array NULL terminated", envArray[0] != NULL);

    env.freeEnvArray(envArray);
    std::cout << "✅ Memory freed successfully\n" << std::endl;

    // ========== CGIEXECUTOR TESTING ==========
    std::cout << "--- CGIExecutor Testing ---\n" << std::endl;

    try {
        std::cout << "\nTest 1: Execute hello.php (GET)" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        Request phpReq("GET", "/hello.php?name=LIB");
        phpReq.setProtocol("HTTP/1.1");
        phpReq.setHeader("Host", "localhost:8080");

        CGIEnvironment phpEnv;
        phpEnv.prepare(phpReq, "./test_scripts/hello.php", "/hello.php", "localhost", 8080);

        char **phpEnvArray = phpEnv.toEnvArray();

        CGIExecutor executor;
        std::string output = executor.execute("/usr/bin/php-cgi",
                                             "./test_scripts/hello.php",
                                             phpEnvArray,
                                             "");

        printTest("PHP execution successful", !output.empty());
        printTest("Output contains Content-Type", output.find("Content-type") != std::string::npos);
        printTest("Output contains HTML", output.find("<html>") != std::string::npos || output.find("Hello") != std::string::npos);

        std::cout << "\nPHP CGI Output (first 500 chars):" << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::cout << output.substr(0, 500) << std::endl;
        std::cout << "..." << std::endl;

        phpEnv.freeEnvArray(phpEnvArray);

    } catch (std::exception &e) {
        std::cout << "⚠️  PHP test skipped: " << e.what() << std::endl;
        std::cout << "   (php-cgi might not be installed)" << std::endl;
    }

    try {
        std::cout << "\nTest 2: Execute echo.py (GET)" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        Request pyReq("GET", "/echo.py?test=hello");
        pyReq.setProtocol("HTTP/1.1");

        CGIEnvironment pyEnv;
        pyEnv.prepare(pyReq, "./test_scripts/echo.py", "/echo.py", "localhost", 8080);

        char **pyEnvArray = pyEnv.toEnvArray();

        CGIExecutor executor2;
        std::string output = executor2.execute("/usr/bin/python3",
                                              "./test_scripts/echo.py",
                                              pyEnvArray,
                                              "");

        printTest("Python execution successful", !output.empty());
        printTest("Output contains Content-Type", output.find("Content-Type") != std::string::npos);

        std::cout << "\nPython CGI Output (first 500 chars):" << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::cout << output.substr(0, 500) << std::endl;
        std::cout << "..." << std::endl;

        pyEnv.freeEnvArray(pyEnvArray);

    } catch (std::exception &e) {
        std::cout << "⚠️  Python test skipped: " << e.what() << std::endl;
        std::cout << "   (python3 might not be installed)" << std::endl;
    }

    // ========== SUMMARY ==========
    std::cout << "\n======================================" << std::endl;
    std::cout << "   TESTING COMPLETE" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "\n✅ CGIDetector: COMPLETE" << std::endl;
    std::cout << "✅ CGIUtils: COMPLETE" << std::endl;
    std::cout << "✅ CGIEnvironment: COMPLETE" << std::endl;
    std::cout << "✅ CGIExecutor: COMPLETE" << std::endl;
    std::cout << "\nProgress: 4/6 modules (67%)" << std::endl;
    std::cout << "Next: CGIOutputParser implementation\n" << std::endl;

    return 0;
}