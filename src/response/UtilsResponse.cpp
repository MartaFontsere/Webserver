#include "../../includes/response/UtilsResponse.hpp"

#include <string>
#include <ctime>
#include <sstream>

/**
 * @file UtilsResponse.cpp
 * @brief HTTP response utility functions implementation
 *
 * Implements C++98-compatible helper functions for HTTP response building.
 * Focuses on RFC-compliant date formatting and type conversion.
 *
 * Key features:
 * - GMT date formatting using POSIX time functions
 * - Safe size_t to string conversion with ostringstream
 * - Zero external dependencies (standard library only)
 *
 * C++98 constraints handled:
 * - No std::to_string → use ostringstream
 * - No std::put_time → use strftime
 * - Must use .c_str() for C-style function calls
 *
 * @see UtilsResponse.hpp for function documentation
 */

/**
 * @brief Generates current date-time in HTTP format (RFC 9110)
 *
 * Implementation using POSIX time functions:
 * 1. time() - Gets current Unix timestamp (seconds since 1970-01-01)
 * 2. gmtime() - Converts timestamp to UTC tm struct
 * 3. strftime() - Formats tm struct to RFC-compliant string
 * 4. Convert char buffer to std::string
 *
 * Flow breakdown:
 *   time_t currentTime;           // Unix timestamp container
 *   time(&currentTime);           // Get current time
 *   struct tm *timeInfo = gmtime(&currentTime); // Convert to UTC
 *   char buffer[80];              // Format buffer
 *   strftime(buffer, 80, format, timeInfo);     // Format to string
 *   return std::string(buffer);   // Convert to C++ string
 *
 * Format string breakdown:
 *   "%a, %d %b %Y %H:%M:%S GMT"
 *
 *   %a  → Abbreviated weekday (Tue)
 *   ,   → Literal comma
 *       → Literal space
 *   %d  → Day of month (16)
 *       → Literal space
 *   %b  → Abbreviated month (Dec)
 *       → Literal space
 *   %Y  → 4-digit year (2025)
 *       → Literal space
 *   %H  → Hour 00-23 (16)
 *   :   → Literal colon
 *   %M  → Minutes 00-59 (30)
 *   :   → Literal colon
 *   %S  → Seconds 00-59 (45)
 *       → Literal space
 *   GMT → Literal (required by RFC)
 *
 * Why gmtime() not localtime():
 * - RFC 9110 Section 5.6.7 mandates UTC/GMT
 * - Local time would break cache validation
 * - Ensures global consistency
 *
 * Buffer size rationale:
 * - Max HTTP date length: ~29 characters
 * - Buffer 80 bytes: safe margin, standard practice
 * - strftime() null-terminates automatically
 *
 * Thread safety note:
 * - gmtime() returns pointer to static struct (not thread-safe)
 * - Acceptable for single-threaded server or with mutex
 * - Alternative: gmtime_r() (POSIX, not standard C++)
 *
 * @return Current date-time string in format "Day, DD Mon YYYY HH:MM:SS GMT"
 *
 * @note Always returns GMT (never CET/PST/etc.)
 * @note Format case-sensitive (browsers are strict)
 * @note Called once per response (fresh timestamp)
 */
std::string getHttpDate()
{
    time_t currentTime;
    time(&currentTime);

    struct tm *timeInfo = gmtime(&currentTime);
    char buffer[80];
    strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);

    std::string result = buffer;
    return result;
}

/**
 * @brief Converts size_t to string (C++98 compatible)
 *
 * Implementation using ostringstream:
 * 1. Create ostringstream object
 * 2. Insert value with << operator (automatic conversion)
 * 3. Extract string with .str() method
 *
 * Flow breakdown:
 *   std::ostringstream oss;  // Output string stream
 *   oss << value;            // Insert number (converts to text)
 *   return oss.str();        // Extract accumulated string
 *
 * Why ostringstream (not alternatives):
 * - sprintf(): C-style, requires buffer size calculation, not type-safe
 * - itoa(): Non-standard (not in C++ spec), not portable
 * - std::to_string(): C++11, not available in C++98
 * - ostringstream: Standard C++98, type-safe, automatic buffering
 *
 * Type safety:
 * - Operator<< overloaded for size_t (no manual conversion)
 * - No buffer overflow risk (ostringstream manages memory)
 * - Works for any numeric type (int, size_t, long, etc.)
 *
 * Performance note:
 * - ostringstream allocates dynamically (small overhead)
 * - Negligible for HTTP response building (called once per response)
 * - Alternative (itoa) would be faster but non-portable
 *
 * Usage patterns:
 *   sizeToString(1234)     → "1234"
 *   sizeToString(0)        → "0"
 *   sizeToString(SIZE_MAX) → "18446744073709551615" (on 64-bit)
 *
 * @param value Unsigned integer to convert (0 to SIZE_MAX)
 * @return Decimal string representation (no thousands separators, no padding)
 *
 * @note No locale formatting (always "1234" not "1,234")
 * @note No leading zeros (123 not "00123")
 * @note Works for 32-bit and 64-bit size_t
 */
std::string sizeToString(size_t value)
{
    std::ostringstream oss;
    oss << value;

    return oss.str();
}