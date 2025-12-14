#include "../../includes/config/UtilsConfig.hpp"

/**
 * @file UtilsConfig.cpp
 * @brief Configuration utility functions - Type conversions and helpers
 *
 * This module provides utility functions for the configuration system,
 * primarily type conversions needed when extracting directive values
 * from BlockParser (which stores all values as strings) and converting
 * them to the appropriate types for ServerConfig/LocationConfig.
 *
 * Current utilities:
 * - stringToInt() - Converts string to integer (C++98 safe)
 *
 * Design rationale:
 * - Centralized conversions prevent code duplication
 * - std::stringstream used instead of atoi/strtol (C++98 standard, safer)
 * - Can be extended with other converters (stringToSizeT, stringToBool, etc.)
 *
 * @note All conversions are C++98 compatible (no C++11 std::stoi)
 */

/**
 * @brief Converts a string to integer using stringstream (C++98 safe)
 *
 * Parses a numeric string and converts it to an integer value. Used primarily
 * for extracting numeric directive values from configuration files.
 *
 * Conversion process:
 * 1. Create stringstream from input string
 * 2. Initialize result variable to 0 (default for invalid input)
 * 3. Extract integer using stream operator >>
 * 4. Return parsed value
 *
 * Error handling:
 * - Invalid input (non-numeric): Returns 0 (stringstream fails silently)
 * - Empty string: Returns 0
 * - Overflow: Undefined behavior (relies on stringstream limits)
 *
 * Common use cases:
 * - Parse port numbers: "8080" → 8080
 * - Parse body size: "1048576" → 1048576
 * - Parse redirect codes: "301" → 301
 * - Parse error codes: "404" → 404
 *
 * Examples:
 *   stringToInt("8080")     → 8080
 *   stringToInt("1024")     → 1024
 *   stringToInt("")         → 0
 *   stringToInt("invalid")  → 0 (fails silently)
 *   stringToInt("42extra")  → 42 (extracts leading number)
 *
 * Why std::stringstream instead of alternatives:
 * - atoi() is C-style, no error reporting
 * - strtol() is C-style, more complex
 * - std::stoi() is C++11 (not available in C++98)
 * - stringstream is idiomatic C++98, type-safe
 *
 * @param value String containing numeric value to convert
 * @return Parsed integer, or 0 if conversion fails
 *
 * @note Returns 0 for invalid input (no exception thrown)
 * @note Partial parsing: "123abc" → 123 (stops at first non-digit)
 * @see getDirectiveValueAsInt() in ConfigBuilder for typical usage
 */
int stringToInt(const std::string &value)
{
    std::stringstream ss(value);
    int number = 0;
    ss >> number;
    return number;
}