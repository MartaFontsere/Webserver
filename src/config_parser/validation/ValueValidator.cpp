#include "../../../includes/config_parser/validation/ValueValidator.hpp"
#include <cctype>
#include <cstdlib>
#include <string>
#include <sstream>

/**
 * @brief Validates if a string contains only digits
 *
 * Checks that every character in the string is a digit (0-9).
 * Does NOT allow negative numbers, decimals, or scientific notation.
 *
 * Valid examples:   "123", "8080", "0", "65535"
 * Invalid examples: "-5", "12.5", "1e3", "abc", "12a"
 *
 * @param value String to validate
 * @return true if all characters are digits, false otherwise
 */
bool isValidNumber(const std::string &value)
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!isdigit(value[i]))
            return false;
    }
    return true;
}

/**
 * @brief Validates if a string is a valid port number
 *
 * Checks:
 * 1. String contains only digits
 * 2. Numerical value is in range [1, 65535]
 *
 * Port 0 is invalid (reserved). Maximum port is 65535 (16-bit limit).
 *
 * Valid examples:   "80", "443", "8080", "3000", "65535"
 * Invalid examples: "0", "99999", "abc", "-80", "8080.5"
 *
 * @param value String to validate as port
 * @return true if valid port number, false otherwise
 */
bool isValidPort(const std::string &value)
{
    if (!isValidNumber(value))
        return false;
    int port = atoi(value.c_str());
    if (port < 1 || port > 65535)
        return false;
    return true;
}

/**
 * @brief Validates if a string is a valid file system path
 *
 * Checks that the path starts with either:
 * - '/' (absolute path): /var/www/html
 * - '.' (relative path): ./html, ../files
 *
 * Does NOT validate:
 * - Path existence on filesystem
 * - Path permissions
 * - Path component validity (special characters, etc.)
 *
 * Valid examples:   "/var/www", "./html", "../static", "/usr/local/nginx"
 * Invalid examples: "html", "var/www", "~user/www"
 *
 * @param value String to validate as path
 * @return true if path starts with / or ., false otherwise
 */
bool isValidPath(const std::string &value)
{
    if (value[0] != '/' && value[0] != '.')
        return false;
    return true;
}

/**
 * @brief Validates if a string is a valid hostname or domain
 *
 * Allows characters commonly found in hostnames:
 * - Alphanumeric (a-z, A-Z, 0-9)
 * - '.' (domain separator): example.com
 * - '-' (hyphen): my-site.com
 * - '*' (wildcard): *.example.com
 * - '_' (underscore, less common)
 * - ':' (port separator): example.com:8080
 * - '~' (tilde, for paths)
 *
 * Note: This is a permissive validation suitable for nginx server_name
 * and similar directives. Does NOT enforce strict DNS hostname rules.
 *
 * Valid examples:   "localhost", "example.com", "*.example.com", "sub-domain.example.com"
 * Invalid examples: "example!.com", "test@host", "domain/.com"
 *
 * @param value String to validate as hostname
 * @return true if all characters are allowed in hostnames, false otherwise
 */
bool isValidHost(const std::string &value)
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        char c = value[i];
        if (!isalnum(c) && c != '.' && c != '-' && c != '*' &&
            c != '_' && c != ':' && c != '~')
            return false;
    }

    return true;
}

/**
 * @brief Validates if a string is a valid IPv4 address
 *
 * Checks IPv4 format (xxx.xxx.xxx.xxx):
 * 1. Contains exactly 3 dots (4 octets)
 * 2. All characters are digits or dots
 * 3. No empty octets (e.g., "192..168.1.1" invalid)
 * 4. Each octet is in range [0, 255]
 * 5. IP doesn't end with dot (e.g., "192.168.1." invalid)
 *
 * Validation steps:
 * - Count dots (must be 3)
 * - Verify only digits and dots
 * - Split by dots and validate each octet
 * - Check octet count is 4
 * - Verify each octet ≤ 255
 *
 * Valid examples:   "127.0.0.1", "192.168.1.1", "0.0.0.0", "255.255.255.255"
 * Invalid examples: "256.1.1.1", "192.168.1", "192..168.1.1", "192.168.1.", "abc.def.ghi.jkl"
 *
 * @param value String to validate as IPv4 address
 * @return true if valid IPv4, false otherwise
 */
bool isValidIP(const std::string &value)
{
    // Count dots (must be exactly 3)
    int dotCount = 0;
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '.')
            dotCount++;
    }
    if (dotCount != 3)
        return false;
    // Verify only digits and dots
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!isdigit(value[i]) && value[i] != '.')
            return false;
    }
    // Validate octets
    std::string oct;
    int octetCount = 0;

    for (size_t i = 0; i < value.size(); ++i)
    {

        if (value[i] == '.')
        {
            if (oct.empty()) // Empty octet (e.g., "192..168")
                return false;
            int octet = atoi(oct.c_str());
            if (octet > 255)
                return false;
            oct.clear();
            octetCount++;
        }
        else
            oct += value[i];
    }
    // Validate last octet
    if (oct.empty()) // IP ends with dot
        return false;
    int octet = atoi(oct.c_str());
    if (octet < 0 || octet > 255)
        return false;
    octetCount++;
    if (octetCount != 4)
        return false;
    return true;
}

/**
 * @brief Validates if a string is a valid HTTP status code
 *
 * Checks:
 * 1. String contains only digits
 * 2. Numerical value is in range [100, 599]
 *
 * HTTP status codes are 3-digit numbers in the range 100-599:
 * - 1xx: Informational
 * - 2xx: Success
 * - 3xx: Redirection
 * - 4xx: Client errors
 * - 5xx: Server errors
 *
 * Valid examples:   "200", "404", "500", "301", "100", "599"
 * Invalid examples: "99", "600", "1000", "abc", "4.04"
 *
 * @param value String to validate as HTTP status code
 * @return true if valid HTTP code (100-599), false otherwise
 */
bool isValidHttpCode(const std::string &value)
{
    if (!isValidNumber(value))
        return false;
    int code = atoi(value.c_str());
    if (code < 100 || code > 599)
        return false;
    return true;
}

/**
 * @brief Validates if a string is a valid nginx boolean value
 *
 * Nginx uses "on" and "off" for boolean directives (NOT true/false).
 * Case-sensitive comparison.
 *
 * Valid examples:   "on", "off"
 * Invalid examples: "true", "false", "yes", "no", "1", "0", "ON", "OFF"
 *
 * @param value String to validate as boolean
 * @return true if value is "on" or "off", false otherwise
 */
bool isValidBool(const std::string &value)
{
    if (value == "on" || value == "off")
        return true;
    return false;
}

/**
 * @brief Validates if a string is a valid nginx location pattern
 *
 * Checks if the pattern starts with a valid location modifier or path:
 * - '/' : Simple path (e.g., "/", "/api", "/images/")
 * - '=' : Exact match (e.g., "= /exact")
 * - '~' : Regex case-sensitive (e.g., "~ \.php$")
 * - '^' : Prefix match (e.g., "^~ /static/")
 *
 * This is a basic check. Full pattern syntax validation
 * (regex correctness, etc.) is not performed.
 *
 * Valid examples:   "/", "/api", "~ \.php$", "= /exact", "^~ /static/"
 * Invalid examples: "$$", "abc", "@test", "test/"
 *
 * @param value String to validate as location pattern
 * @return true if starts with valid location modifier, false otherwise
 */
bool isValidPattern(const std::string &value)
{
    return (value[0] == '/' || value[0] == '~' || value[0] == '=' || value[0] == '^');
}