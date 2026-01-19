#include "../../includes/cgi/CGIUtils.hpp"

/**
 * @file CGIUtils.cpp
 * @brief Utility functions for CGI module (C++98 compliant)
 *
 * This file provides helper functions for string manipulation, type conversion,
 * and HTTP header processing specifically for CGI operations. All functions
 * avoid C-style functions (strcpy, itoa, sprintf) in favor of C++ equivalents
 * to maintain standard compliance and type safety.
 *
 * Key design decisions:
 * - std::stringstream instead of itoa/sprintf (not standard in C++98)
 * - Manual character-by-character copying instead of strcpy (C++ purity)
 * - Free functions instead of static class methods (more idiomatic for
 * utilities)
 * - const correctness enforced (const& parameters when not modifying)
 *
 * @note All functions are compatible with C++98 standard
 * @see RFC 3875 (CGI/1.1) for HTTP header to environment variable conversion
 * rules
 */

/**
 * @brief Converts integer to string (C++98 compatible alternative to
 * std::to_string)
 *
 * Uses std::stringstream for conversion since std::to_string was introduced
 * in C++11. This implementation is type-safe and locale-aware.
 *
 * Implementation rationale:
 * - Avoids itoa() (non-standard, not in C++98)
 * - Avoids sprintf() (C-style, buffer overflow risks)
 * - Uses stringstream (standard C++98, safe)
 *
 * Example usage:
 *   intToString(8080) → "8080"
 *   intToString(-42)  → "-42"
 *   intToString(0)    → "0"
 *
 * @param value Integer value to convert
 * @return String representation of the integer
 */

std::string intToString(int value) {
  std::stringstream stringResult;
  stringResult << value;
  return stringResult.str();
}

/**
 * @brief Extracts query string from a URI (everything after '?')
 *
 * Searches for the first '?' character and returns everything after it
 * (excluding the '?' itself). Returns empty string if no query string present.
 *
 * Query string format (RFC 3986):
 *   URI = scheme://host/path?query#fragment
 *         \_________________/\____/
 *              path part      query
 *
 * Examples:
 *   "/script.php?name=world&id=42"  → "name=world&id=42"
 *   "/index.html"                   → ""
 *   "/api?key="                     → "key="
 *   "/test?"                        → ""
 *
 * @param uri Complete URI string (may include query parameters)
 * @return Query string without '?' or empty string if none
 *
 * @note Uses std::string::npos validation (not just if(pos)) to handle
 *       edge case where '?' is at position 0
 */

std::string extractQueryString(const std::string &uri) {
  size_t questPos = uri.find('?');
  if (questPos != std::string::npos) {
    std::string result = uri.substr(questPos + 1);
    return result;
  }
  return "";
}

/**
 * @brief Converts all HTTP headers to CGI environment variable format
 *
 * Transforms a map of HTTP headers into CGI-compliant environment variables
 * according to RFC 3875 section 4.1.18. Each header is converted by:
 * 1. Converting to uppercase
 * 2. Replacing hyphens with underscores
 * 3. Prefixing with "HTTP_"
 *
 * Conversion examples:
 *   "Host"         → "HTTP_HOST"
 *   "User-Agent"   → "HTTP_USER_AGENT"
 *   "Content-Type" → "HTTP_CONTENT_TYPE"
 *   "Accept"       → "HTTP_ACCEPT"
 *
 * Complete example:
 *   Input:  {"Host": "localhost", "User-Agent": "Chrome/120"}
 *   Output: {"HTTP_HOST": "localhost", "HTTP_USER_AGENT": "Chrome/120"}
 *
 * @param headers Map of HTTP header names to values (original format)
 * @return New map with CGI environment variable names as keys
 *
 * @note Uses const_iterator for const correctness (parameter is const&)
 * @see headerToEnvName() for single header conversion logic
 */

std::map<std::string, std::string>
convertHeadersToEnv(const std::map<std::string, std::string> &headers) {
  std::map<std::string, std::string> group;

  std::map<std::string, std::string>::const_iterator it;
  for (it = headers.begin(); it != headers.end(); ++it) {
    std::string envName = headerToEnvName(it->first);
    group[envName] = it->second;
  }

  return group;
}

/**
 * @brief Converts a single HTTP header name to CGI environment variable format
 *
 * Performs the transformation required by RFC 3875:
 * 1. Replace all hyphens (-) with underscores (_)
 * 2. Convert entire string to uppercase
 * 3. Prepend "HTTP_" prefix
 *
 * Algorithm:
 * - Character-by-character iteration (C++98 style, no range-based for)
 * - Conditional replacement for hyphen detection
 * - Uses toUpperCase() helper for case conversion
 *
 * Transformation examples:
 *   "User-Agent"     → "HTTP_USER_AGENT"
 *   "Accept-Encoding"→ "HTTP_ACCEPT_ENCODING"
 *   "X-Custom-Header"→ "HTTP_X_CUSTOM_HEADER"
 *   "Host"           → "HTTP_HOST"
 *
 * @param headerName Original HTTP header name (case-insensitive in HTTP, but
 * preserved)
 * @return CGI environment variable name (uppercase with HTTP_ prefix)
 *
 * @note Special CGI variables (Content-Type, Content-Length) should NOT use
 * this function - they become CONTENT_TYPE and CONTENT_LENGTH without HTTP_
 * prefix
 * @see RFC 3875 section 4.1.18 (Protocol-Specific Meta-Variables)
 */

std::string headerToEnvName(const std::string &headerName) {
  std::string result;
  for (int i = 0; i < (int)headerName.length(); i++) {
    if (headerName[i] == '-')
      result += '_';
    else
      result += headerName[i];
  }
  return "HTTP_" + toUpperCase(result);
}

/**
 * @brief Converts a string to uppercase
 *
 * Creates a new string with all alphabetic characters converted to uppercase.
 * Non-alphabetic characters are preserved unchanged. Uses std::toupper from
 * <cctype> for locale-aware conversion.
 *
 * Implementation notes:
 * - Cannot modify parameter (const& for correctness)
 * - Creates new string instead of in-place modification
 * - Character-by-character iteration (C++98 compatible)
 * - Uses int cast for length() to avoid size_t comparison warnings with -Wextra
 *
 * Examples:
 *   "hello"        → "HELLO"
 *   "User-Agent"   → "USER-AGENT"
 *   "http_host"    → "HTTP_HOST"
 *   "Test123"      → "TEST123"
 *
 * @param str Input string to convert (not modified)
 * @return New string with all characters in uppercase
 *
 * @note std::toupper is locale-dependent; assumes ASCII in CGI context
 */

std::string toUpperCase(const std::string &str) {
  std::string result;
  for (int i = 0; i < (int)str.length(); i++)
    result += std::toupper(str[i]);
  return result;
}

/**
 * @brief Converts std::string to C-style string (char array)
 *
 * Manually copies characters from std::string to a pre-allocated char array,
 * adding null terminator at the end. This function is necessary for interfacing
 * with POSIX system calls (execve) that require char** arrays.
 *
 * Why not use strcpy():
 * - Maintains C++ purity (avoids C standard library when possible)
 * - Explicit character-by-character copy is clearer
 * - No dependency on <cstring> (already avoided in C++98 style)
 *
 * Memory requirements:
 * - Caller MUST allocate dest with size >= source.size() + 1
 * - Recommended: dest = new char[source.size() + 1]
 * - Function does NOT allocate memory (caller's responsibility)
 *
 * Usage example:
 *   std::string str = "REQUEST_METHOD=GET";
 *   char *buffer = new char[str.size() + 1];
 *   stringToCString(str, buffer);
 *   // buffer now contains "REQUEST_METHOD=GET\0"
 *   // ... use buffer with execve() ...
 *   delete[] buffer;
 *
 * @param source Source std::string to convert
 * @param dest Pre-allocated char array (must have space for size() + 1)
 *
 * @warning Caller must ensure dest has sufficient space - NO bounds checking
 * @note Always adds '\0' terminator at position source.size()
 * @see CGIEnvironment::toEnvArray() for primary use case
 */

void stringToCString(const std::string &source, char *dest) {
  for (size_t i = 0; i < source.size(); ++i) {
    dest[i] = source[i];
  }
  dest[source.size()] = '\0';
}