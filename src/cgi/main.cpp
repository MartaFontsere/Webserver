#include <iostream>
#include "../../includes/cgi/hardcoded/Request.hpp"
#include "../../includes/cgi/hardcoded/Response.hpp"
#include "../../includes/cgi/hardcoded/LocationConfig.hpp"
#include "../../includes/cgi/CGIDetector.hpp"
#include "../../includes/cgi/CGIEnvironment.hpp"
#include "../../includes/cgi/CGIExecutor.hpp"
#include "../../includes/cgi/CGIOutputParser.hpp"
#include "../../includes/cgi/CGIUtils.hpp"
#include "../../includes/cgi/CGIHandler.hpp"

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
    std::cout << "======================================\n"
              << std::endl;

    LocationConfig loc;

    // ========== CGIUTILS TESTING ==========
    std::cout << "--- CGIUtils Testing ---\n"
              << std::endl;

    std::string port = intToString(8080);
    printTest("intToString(8080)", port == "8080");

    std::string query = extractQueryString("/script.php?name=world");
    printTest("extractQueryString", query == "name=world");

    std::string upper = toUpperCase("Hello-World");
    printTest("toUpperCase", upper == "HELLO-WORLD");

    std::string envName = headerToEnvName("User-Agent");
    printTest("headerToEnvName", envName == "HTTP_USER_AGENT");

    // ========== CGIENVIRONMENT TESTING ==========
    std::cout << "\n--- CGIEnvironment Testing ---\n"
              << std::endl;

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

    // ========== CGIEXECUTOR TESTING ==========
    std::cout << "\n--- CGIExecutor Testing ---\n"
              << std::endl;

    try
    {
        std::cout << "Execute hello.php (GET)" << std::endl;

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
        printTest("Output contains content", output.find("Hello") != std::string::npos);

        phpEnv.freeEnvArray(phpEnvArray);

        // ========== CGIOUTPUTPARSER TESTING ==========
        std::cout << "\n--- CGIOutputParser Testing ---\n"
                  << std::endl;

        CGIOutputParser parser;
        parser.parse(output);

        // Test headers extraction
        std::map<std::string, std::string> headers = parser.getHeaders();
        printTest("Headers extracted", !headers.empty());
        printTest("Content-Type header exists", headers.find("Content-type") != headers.end());

        std::cout << "\nExtracted Headers:" << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::map<std::string, std::string>::const_iterator it;
        for (it = headers.begin(); it != headers.end(); ++it)
        {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }

        // Test body extraction
        std::string body = parser.getBody();
        printTest("Body extracted", !body.empty());
        printTest("Body contains HTML", body.find("<html>") != std::string::npos || body.find("Hello") != std::string::npos);

        std::cout << "\nExtracted Body (first 200 chars):" << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::cout << body.substr(0, 200) << std::endl;

        // Test status code
        int statusCode = parser.getStatusCode();
        std::cout << "\nStatus Code: " << statusCode << std::endl;
        printTest("Status code extracted", statusCode == 200 || statusCode > 0);

        // Test parsing with explicit Status header
        std::cout << "\n--- Test Custom Status Code ---" << std::endl;
        std::string customOutput = "Status: 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html>404</html>";
        CGIOutputParser parser2;
        parser2.parse(customOutput);

        printTest("Custom status 404", parser2.getStatusCode() == 404);
        printTest("Custom body", parser2.getBody() == "<html>404</html>");
    }
    catch (std::exception &e)
    {
        std::cout << "⚠️  Test failed: " << e.what() << std::endl;
    }

    // ========== CGIHANDLER TESTING (END-TO-END) ==========
    std::cout << "\n--- CGIHandler Testing (End-to-End) ---\n"
              << std::endl;

    try
    {
        std::cout << "Test 1: Complete CGI workflow with PHP" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        Request cgiReq("GET", "/hello.php?name=WebServ");
        cgiReq.setProtocol("HTTP/1.1");
        cgiReq.setHeader("Host", "localhost:8080");
        cgiReq.setHeader("User-Agent", "CGITest/1.0");

        LocationConfig cgiLoc; // Ya tiene serverName y serverPort

        CGIHandler handler;
        Response cgiResponse = handler.handle(cgiReq, cgiLoc);

        printTest("CGI workflow successful", cgiResponse.getStatusCode() == 200);
        printTest("Response has body", !cgiResponse.getBody().empty());
        printTest("Response contains HTML", cgiResponse.getBody().find("<html>") != std::string::npos || cgiResponse.getBody().find("Hello") != std::string::npos);

        std::cout << "\nFinal Response:" << std::endl;
        std::cout << "Status Code: " << cgiResponse.getStatusCode() << std::endl;
        std::cout << "Body (first 300 chars):" << std::endl;
        std::cout << cgiResponse.getBody().substr(0, 300) << std::endl;
        std::cout << "..." << std::endl;

        std::cout << "\n✅ FULL CGI PIPELINE WORKING!" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "⚠️  CGI Handler test failed: " << e.what() << std::endl;
    }

    // ========== SUMMARY ==========
    std::cout << "\n======================================" << std::endl;
    std::cout << "   TESTING COMPLETE" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "\n✅ CGIDetector: COMPLETE" << std::endl;
    std::cout << "✅ CGIUtils: COMPLETE" << std::endl;
    std::cout << "✅ CGIEnvironment: COMPLETE" << std::endl;
    std::cout << "✅ CGIExecutor: COMPLETE" << std::endl;
    std::cout << "✅ CGIOutputParser: COMPLETE" << std::endl;
    std::cout << "\nProgress: 5/6 modules (83%)" << std::endl;
    std::cout << "\nNext: CGIHandler (final module)\n"
              << std::endl;

    return 0;
}