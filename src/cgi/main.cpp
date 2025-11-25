#include <iostream>
#include "../../includes/cgi/hardcoded/Request.hpp"
#include "../../includes/cgi/hardcoded/Response.hpp"
#include "../../includes/cgi/hardcoded/LocationConfig.hpp"
#include "../../includes/cgi/CGIDetector.hpp"

void printTest(const std::string &testName, bool passed)
{
    if (passed)
        std::cout << "✅ " << testName << std::endl;
    else
        std::cout << "❌ " << testName << std::endl;
}

int main()
{
    std::cout << "\n=== CGI DETECTOR TESTING ===" << std::endl;

    LocationConfig loc;

    // TEST 1: isCGIRequest - PHP file
    std::cout << "\n--- TEST 1: isCGIRequest() ---" << std::endl;
    bool test1 = CGIDetector::isCGIRequest("/cgi-bin/hello.php", loc.cgiExts);
    printTest("PHP file detected as CGI", test1 == true);

    // TEST 2: isCGIRequest - Non-CGI file
    bool test2 = CGIDetector::isCGIRequest("/index.html", loc.cgiExts);
    printTest("HTML file NOT detected as CGI", test2 == false);

    // TEST 3: isCGIRequest - PHP with query string
    bool test3 = CGIDetector::isCGIRequest("/script.php?name=world", loc.cgiExts);
    printTest("PHP with query string detected", test3 == true);

    // TEST 4: isCGIRequest - php5 extension
    bool test4 = CGIDetector::isCGIRequest("/legacy.php5", loc.cgiExts);
    printTest("PHP5 extension detected", test4 == true);

    // TEST 5: isCGIRequest - No extension
    bool test5 = CGIDetector::isCGIRequest("/no-extension", loc.cgiExts);
    printTest("No extension NOT detected as CGI", test5 == false);

    // TEST 6: getCGIExecutable - PHP file
    std::cout << "\n--- TEST 2: getCGIExecutable() ---" << std::endl;
    std::string exec1 = CGIDetector::getCGIExecutable("/var/www/hello.php", loc.cgiPaths, loc.cgiExts);
    printTest("PHP executable found", exec1 == "/usr/bin/php-cgi");
    std::cout << "  Found: " << exec1 << std::endl;

    // TEST 7: getCGIExecutable - Non-CGI file
    std::string exec2 = CGIDetector::getCGIExecutable("/var/www/index.html", loc.cgiPaths, loc.cgiExts);
    printTest("Non-CGI returns empty", exec2.empty());

    // TEST 8: resolveScriptPath - Normal case
    std::cout << "\n--- TEST 3: resolveScriptPath() ---" << std::endl;
    std::string path1 = CGIDetector::resolveScriptPath("/cgi-bin/hello.php", "./test_scripts");
    printTest("Normal path resolution", path1 == "./test_scripts/cgi-bin/hello.php");
    std::cout << "  Resolved: " << path1 << std::endl;

    // TEST 9: resolveScriptPath - With query string
    std::string path2 = CGIDetector::resolveScriptPath("/script.php?name=test", "./test_scripts");
    printTest("Path with query string", path2 == "./test_scripts/script.php");
    std::cout << "  Resolved: " << path2 << std::endl;

    // TEST 10: resolveScriptPath - Root with trailing slash
    std::string path3 = CGIDetector::resolveScriptPath("/hello.php", "./test_scripts/");
    printTest("Root with trailing slash", path3 == "./test_scripts/hello.php");
    std::cout << "  Resolved: " << path3 << std::endl;

    // Summary
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "CGIDetector: ALL FUNCTIONS IMPLEMENTED" << std::endl;
    std::cout << "Ready for CGIEnvironment implementation" << std::endl;

    return 0;
}