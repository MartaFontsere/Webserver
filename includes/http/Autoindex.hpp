#pragma once

#include <string>

/**
 * @brief Autoindex utilities - generates directory listings as HTML
 */
namespace Autoindex {

/** @brief Generate HTML listing for a directory */
std::string generateListing(const std::string &dirPath,
                            const std::string &urlPath);

/** @brief Escape HTML special characters for security */
std::string escapeHtml(const std::string &input);

/** @brief URL-encode a string for use in href attributes */
std::string urlEncode(const std::string &input);

} // namespace Autoindex