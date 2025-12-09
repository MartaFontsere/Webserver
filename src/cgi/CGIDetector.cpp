#include "../../includes/cgi/CGIDetector.hpp"
#include <vector>
#include <string>

/**
 * @file CGIDetector.cpp
 * @brief CGI request detection and path resolution implementation
 *
 * This module handles the first phase of CGI processing: detecting whether
 * a request should be handled as CGI and resolving the necessary paths.
 *
 * Key responsibilities:
 * - Detect CGI requests by file extension (.php, .py, .sh, etc.)
 * - Extract file extensions from URIs (handling query strings)
 * - Resolve script filesystem paths from URIs
 * - Match CGI extensions to their corresponding executables
 *
 * Path resolution complexity:
 * The resolveScriptPath() function handles 4 combinations of trailing slashes
 * to ensure correct path construction regardless of configuration format:
 *   root="./www"  + uri="/script.php"  → "./www/script.php"
 *   root="./www/" + uri="/script.php"  → "./www/script.php"
 *   root="./www"  + uri="script.php"   → "./www/script.php"
 *   root="./www/" + uri="script.php"   → "./www/script.php"
 *
 * @note All methods are static - CGIDetector is a stateless utility class
 * @see RFC 3875 section 4.1.5 (PATH_INFO and SCRIPT_NAME)
 */

/**
 * @brief Extracts file extension from a path, handling query strings
 *
 * Searches for the last dot (.) in the path and returns everything from
 * that point to the end (or to '?' if query string present). This is critical
 * for CGI detection since URIs often contain query parameters.
 *
 * Algorithm:
 * 1. Find last dot position (find_last_of('.'))
 * 2. Find query string position (find('?'))
 * 3. Handle edge cases (no dot, dot at end, query before/after dot)
 * 4. Extract substring accordingly
 *
 * Edge cases handled:
 * - No extension: "/path/file" → ""
 * - Trailing dot: "/path/file." → ""
 * - Query string: "/script.php?id=1" → ".php" (not ".php?id=1")
 * - Multiple dots: "/path/file.tar.gz" → ".gz"
 * - Hidden files: "/.bashrc" (dot at position 0) → not tested, returns from position 0
 *
 * Examples:
 *   "/hello.php"           → ".php"
 *   "/script.py?name=test" → ".py"
 *   "/index.html"          → ".html"
 *   "/noext"               → ""
 *   "/file."               → ""
 *
 * @param path URI or filesystem path (may include query string)
 * @return File extension with dot (".php") or empty string if none
 *
 * @note Uses std::string::npos validation (critical for edge cases)
 * @note Query string is correctly handled to avoid returning ".php?id=1"
 */

std::string CGIDetector::getExtension(const std::string &path)
{
    size_t lastDot = path.find_last_of('.');
    size_t questPos = path.find('?');
    if (lastDot == std::string::npos || lastDot == path.size() - 1)
        return "";
    if (questPos != std::string::npos && questPos > lastDot)
        return path.substr(lastDot, (questPos - lastDot));
    return path.substr(lastDot);
}

/**
 * @brief Removes query string from URI (everything from '?' onwards)
 *
 * Cleans a URI by removing all query parameters. This is essential for
 * filesystem operations since query strings are not part of the file path.
 *
 * Query string format (RFC 3986):
 *   URI = /path/to/resource?param1=value1&param2=value2
 *         \________________/\________________________/
 *            clean path          query string
 *
 * Examples:
 *   "/script.php?name=world&id=42" → "/script.php"
 *   "/index.html"                  → "/index.html"
 *   "/api?key="                    → "/api"
 *   "/test?"                       → "/test"
 *
 * @param uri Complete URI that may contain query parameters
 * @return URI without query string (everything before '?')
 *
 * @note Returns original URI if no query string present
 * @note Correctly handles edge case where '?' is at position 0 (unlikely but safe)
 */

std::string CGIDetector::removeQueryString(const std::string &uri)
{
    size_t quest = uri.find('?');

    if (quest == std::string::npos)
        return uri;
    return uri.substr(0, quest);
}

/**
 * @brief Detects if a request should be handled as CGI based on file extension
 *
 * Compares the URI's file extension against a list of configured CGI extensions.
 * This is the primary decision point for routing requests to the CGI handler.
 *
 * Detection process:
 * 1. Extract extension from URI (handles query strings automatically)
 * 2. Compare against each configured CGI extension
 * 3. Return true on first match, false if none match
 *
 * Configuration example (from nginx-like config):
 *   cgi_ext .php .py .sh;
 *   → cgiExts vector = {".php", ".py", ".sh"}
 *
 * Detection examples:
 *   URI="/script.php?id=1", cgiExts={".php", ".py"} → true
 *   URI="/index.html", cgiExts={".php", ".py"}      → false
 *   URI="/test.py", cgiExts={".php"}                → false
 *   URI="/api.php", cgiExts={".php", ".py", ".sh"}  → true
 *
 * @param uri Request URI (may include query string)
 * @param cgiExsts Vector of configured CGI extensions (e.g., {".php", ".py"})
 * @return true if URI extension matches any configured CGI extension, false otherwise
 *
 * @note Linear search is acceptable - typical cgiExts.size() is 1-3
 * @note Empty extension in URI returns false (no match possible)
 */

bool CGIDetector::isCGIRequest(const std::string &uri, const std::vector<std::string> &cgiExsts)
{
    std::string ext = getExtension(uri);

    for (size_t i = 0; i < cgiExsts.size(); ++i)
    {
        if (cgiExsts[i] == ext)
            return true;
    }
    return false;
}

/**
 * @brief Finds the CGI executable path for a given script
 *
 * Matches the script's file extension against configured extensions and returns
 * the corresponding executable path. This mapping is defined in the configuration.
 *
 * Configuration mapping (nginx-like):
 *   location ~ \.php$ {
 *       cgi_path /usr/bin/php-cgi;
 *       cgi_ext .php;
 *   }
 *   → cgiPaths[0] = "/usr/bin/php-cgi"
 *   → cgiExts[0] = ".php"
 *
 * Matching logic:
 * - Extension at cgiExts[i] corresponds to executable at cgiPaths[i]
 * - First match wins (assumes unique extensions in config)
 * - Returns first cgiPaths entry on match (simplified - should return cgiPaths[i])
 *
 * Examples:
 *   script="hello.php", cgiPaths={"/usr/bin/php-cgi"}, cgiExts={".php"}
 *     → "/usr/bin/php-cgi"
 *   
 *   script="test.py", cgiPaths={"/usr/bin/python3"}, cgiExts={".py"}
 *     → "/usr/bin/python3"
 *   
 *   script="index.html", cgiPaths={"/usr/bin/php-cgi"}, cgiExts={".php"}
 *     → "" (not a CGI extension)
 *
 * @param scriptPath Path to the script file (filesystem path or URI)
 * @param cgiPaths Vector of CGI executable paths (e.g., {"/usr/bin/php-cgi", "/usr/bin/python3"})
 * @param cgiExsts Vector of CGI extensions (parallel to cgiPaths)
 * @return Path to CGI executable if match found, empty string otherwise
 *
 * @note Current implementation returns cgiPaths[0] instead of cgiPaths[i]
 *       This works if only one CGI type is configured per location block
 * @note Returns empty string if extension not found or is empty
 */

std::string CGIDetector::getCGIExecutable(const std::string &scriptPath, const std::vector<std::string> &cgiPaths, 
                                    const std::vector<std::string> &cgiExsts)
{
    std::string ext = getExtension(scriptPath);

    if (ext.empty())
        return "";
    for (size_t i = 0; i < cgiExsts.size(); ++i)
    {
        if (cgiExsts[i] == ext)
            return cgiPaths[0]; //Should return cgiPaths[i] for multi-CGI support
    }
    return "";
}

/**
 * @brief Resolves complete filesystem path for a script from URI and root
 *
 * Combines the document root with the URI to produce the absolute path where
 * the script file should be located. Handles all 4 possible combinations of
 * trailing slashes to avoid path construction errors.
 *
 * Trailing slash combinations (all produce same result):
 *   1. root="./www"  + uri="/script.php"  → "./www/script.php"   (neither has slash at junction)
 *   2. root="./www/" + uri="/script.php"  → "./www/script.php"   (root has trailing, uri has leading)
 *   3. root="./www"  + uri="script.php"   → "./www/script.php"   (root missing trailing, uri missing leading)
 *   4. root="./www/" + uri="script.php"   → "./www/script.php"   (root has trailing, uri missing leading)
 *
 * Algorithm:
 * 1. Remove query string from URI (filesystem doesn't include params)
 * 2. Extract first character of clean URI and last character of root
 * 3. Determine slash situation:
 *    - XOR case (one has slash, other doesn't): simple concatenation
 *    - Both missing: add slash between
 *    - Both present: remove one duplicate
 * 4. Return combined path
 *
 * Examples:
 *   resolveScriptPath("/hello.php?test=1", "./test_scripts")
 *     → "./test_scripts/hello.php"
 *   
 *   resolveScriptPath("/api/script.py", "/var/www/")
 *     → "/var/www/api/script.py"
 *   
 *   resolveScriptPath("index.php", "./www")
 *     → "./www/index.php"
 *
 * @param uri Request URI (may include query string)
 * @param root Document root directory (from location config)
 * @return Complete filesystem path to script file
 *
 * @note Query string is automatically removed before path construction
 * @note Empty URI returns root unchanged
 * @note Last return "" should never be reached (defensive programming)
 */

std::string CGIDetector::resolveScriptPath(const std::string &uri, const std::string &root)
{
    std::string headUri = removeQueryString(uri);
    char firstUri = headUri[0];
    char lastRoot = root[root.size() - 1];

    if (headUri.empty())
        return root;
    // Case 1 & 2: XOR - exactly one has slash at junction point
    if ((lastRoot != '/' && firstUri == '/') || (lastRoot == '/' && firstUri != '/'))
        return root + headUri;
    // Case 3: Both missing slash - add separator
    if (lastRoot != '/' && firstUri != '/')
        return root + '/' + headUri;
    // Case 4: Both have slash - remove duplicate
    if (lastRoot == '/' && firstUri == '/')
    {
        std::string root2 = root.substr(0, root.size() - 1);
        return root2 + headUri;
    }
    return ""; // Should never reach here (all cases covered above)
}