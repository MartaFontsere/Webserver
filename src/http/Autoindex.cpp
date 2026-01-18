#include "http/Autoindex.hpp"
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @file Autoindex.cpp
 * @brief Directory listing generation for web server
 *
 * This module generates HTML directory listings when:
 * - The requested URL points to a directory
 * - No index file (e.g., index.html) exists
 * - Autoindex is enabled in the location config
 *
 * Features:
 * - Modern dark theme styling
 * - File type icons based on extension
 * - File size formatting (B/KB/MB)
 * - Last modification timestamps
 * - Parent directory navigation (..)
 * - URL encoding for special characters
 * - HTML escaping to prevent XSS
 * - Entry limit for security (1000 max)
 *
 * @see StaticFileHandler for directory handling
 */

namespace Autoindex {

/**
 * @brief Generates an HTML directory listing
 *
 * Creates a styled HTML page listing all files and subdirectories
 * in the specified directory.
 *
 * @param dirPath Absolute filesystem path to the directory
 * @param urlPath URL path as requested by client (for links and title)
 * @return HTML string, or empty string if directory can't be opened
 *
 * @note Returns empty string on error; caller should check errno
 * @note Limits output to MAX_ENTRIES (1000) for security
 */
std::string generateListing(const std::string &dirPath,
                            const std::string &urlPath) {
  DIR *dir = opendir(dirPath.c_str());
  if (!dir) {
    return "";
  }

  std::ostringstream html;
  std::string safeUrlPath = escapeHtml(urlPath);

  // HTML header with modern dark theme CSS
  html << "<!DOCTYPE html>\n"
       << "<html>\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <title>Index of " << safeUrlPath << "</title>\n"
       << "  <style>\n"
       << "    * { box-sizing: border-box; margin: 0; padding: 0; }\n"
       << "    body {\n"
       << "      font-family: 'Segoe UI', system-ui, sans-serif;\n"
       << "      background: linear-gradient(135deg, #0f172a 0%, #1e293b "
          "100%);\n"
       << "      color: #f8fafc;\n"
       << "      min-height: 100vh;\n"
       << "      padding: 2rem;\n"
       << "    }\n"
       << "    .container {\n"
       << "      max-width: 900px;\n"
       << "      margin: 0 auto;\n"
       << "      background: rgba(30, 41, 59, 0.8);\n"
       << "      border-radius: 1rem;\n"
       << "      padding: 2rem;\n"
       << "      box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);\n"
       << "    }\n"
       << "    h1 {\n"
       << "      color: #38bdf8;\n"
       << "      margin-bottom: 1.5rem;\n"
       << "      font-size: 1.5rem;\n"
       << "      display: flex;\n"
       << "      align-items: center;\n"
       << "      gap: 0.5rem;\n"
       << "    }\n"
       << "    table {\n"
       << "      width: 100%;\n"
       << "      border-collapse: collapse;\n"
       << "    }\n"
       << "    th {\n"
       << "      text-align: left;\n"
       << "      padding: 0.75rem 1rem;\n"
       << "      background: rgba(56, 189, 248, 0.1);\n"
       << "      color: #94a3b8;\n"
       << "      font-weight: 600;\n"
       << "      font-size: 0.8rem;\n"
       << "      text-transform: uppercase;\n"
       << "      letter-spacing: 0.05em;\n"
       << "    }\n"
       << "    td {\n"
       << "      padding: 0.75rem 1rem;\n"
       << "      border-bottom: 1px solid rgba(148, 163, 184, 0.1);\n"
       << "    }\n"
       << "    tr:hover td {\n"
       << "      background: rgba(56, 189, 248, 0.05);\n"
       << "    }\n"
       << "    a {\n"
       << "      text-decoration: none;\n"
       << "      color: #f8fafc;\n"
       << "      display: flex;\n"
       << "      align-items: center;\n"
       << "      gap: 0.5rem;\n"
       << "    }\n"
       << "    a:hover { color: #38bdf8; }\n"
       << "    .size { text-align: right; color: #64748b; }\n"
       << "    .date { color: #64748b; }\n"
       << "    .dir a { color: #fbbf24; font-weight: 500; }\n"
       << "    .icon { font-size: 1.1rem; }\n"
       << "    footer {\n"
       << "      margin-top: 1.5rem;\n"
       << "      padding-top: 1rem;\n"
       << "      border-top: 1px solid rgba(148, 163, 184, 0.1);\n"
       << "      color: #64748b;\n"
       << "      font-size: 0.8rem;\n"
       << "      text-align: center;\n"
       << "    }\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <div class=\"container\">\n"
       << "    <h1>🗂️ Index of " << safeUrlPath << "</h1>\n"
       << "    <table>\n"
       << "      <tr>\n"
       << "        <th>Name</th>\n"
       << "        <th>Last Modified</th>\n"
       << "        <th class=\"size\">Size</th>\n"
       << "      </tr>\n";

  // Parent directory link (if not at root)
  if (urlPath != "/" && !urlPath.empty()) {
    std::string parentPath = urlPath;
    if (parentPath[parentPath.size() - 1] == '/')
      parentPath.erase(parentPath.size() - 1);
    size_t lastSlash = parentPath.find_last_of('/');

    if (lastSlash == std::string::npos)
      parentPath = "/";
    else
      parentPath = parentPath.substr(0, lastSlash + 1);

    html << "      <tr class=\"dir\">\n"
         << "        <td><a href=\"" << parentPath
         << "\"><span class=\"icon\">⬆️</span> ../</a></td>\n"
         << "        <td class=\"date\">-</td>\n"
         << "        <td class=\"size\">-</td>\n"
         << "      </tr>\n";
  }

  // Read directory entries
  struct dirent *entry;
  int entryCount = 0;
  const int MAX_ENTRIES = 1000;

  while ((entry = readdir(dir)) != NULL && entryCount < MAX_ENTRIES) {
    std::string name = entry->d_name;

    // Skip . and .. (parent handled above)
    if (name == "." || name == "..")
      continue;

    // Get file metadata
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/')
      fullPath += "/";
    fullPath += name;

    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) == 0) {
      bool isDirectory = S_ISDIR(fileStat.st_mode);

      // Format modification date
      char dateBuf[64];
      struct tm *timeinfo = localtime(&fileStat.st_mtime);
      if (timeinfo)
        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", timeinfo);
      else
        std::strcpy(dateBuf, "-");

      // Format file size
      std::string sizeStr;
      if (isDirectory) {
        sizeStr = "-";
      } else {
        if (fileStat.st_size < 1024) {
          std::ostringstream oss;
          oss << fileStat.st_size << " B";
          sizeStr = oss.str();
        } else if (fileStat.st_size < 1024 * 1024) {
          std::ostringstream oss;
          oss << (fileStat.st_size / 1024) << " KB";
          sizeStr = oss.str();
        } else {
          std::ostringstream oss;
          oss << (fileStat.st_size / (1024 * 1024)) << " MB";
          sizeStr = oss.str();
        }
      }

      // Prepare display name (escaped, with / for directories)
      std::string displayName = escapeHtml(name);
      if (isDirectory)
        displayName += "/";

      // URL-encode filename for link
      std::string urlEncodedName = urlEncode(name);

      // Determine icon based on file type/extension
      std::string icon = isDirectory ? "📁" : "📄";
      if (!isDirectory) {
        size_t dotPos = name.rfind('.');
        if (dotPos != std::string::npos) {
          std::string ext = name.substr(dotPos);
          if (ext == ".html" || ext == ".htm")
            icon = "🌐";
          else if (ext == ".css")
            icon = "🎨";
          else if (ext == ".js")
            icon = "⚡";
          else if (ext == ".png" || ext == ".jpg" || ext == ".gif" ||
                   ext == ".webp")
            icon = "🖼️";
          else if (ext == ".pdf")
            icon = "📝";
          else if (ext == ".txt")
            icon = "📄";
          else if (ext == ".py" || ext == ".sh" || ext == ".cpp" || ext == ".c")
            icon = "💻";
        }
      }

      html << "      <tr>\n"
           << "        <td class=\"" << (isDirectory ? "dir" : "") << "\">"
           << "<a href=\"" << urlEncodedName << (isDirectory ? "/" : "")
           << "\"><span class=\"icon\">" << icon << "</span> " << displayName
           << "</a></td>\n"
           << "        <td class=\"date\">" << dateBuf << "</td>\n"
           << "        <td class=\"size\">" << sizeStr << "</td>\n"
           << "      </tr>\n";
      entryCount++;
    }
  }

  closedir(dir);

  // Show warning if entry limit reached
  if (entryCount >= MAX_ENTRIES) {
    html << "    <tr>\n"
         << "      <td colspan=\"3\" style=\"color: #666; font-style: "
            "italic;\">"
         << "(Showing first " << MAX_ENTRIES << " entries)</td>\n"
         << "    </tr>\n";
  }

  // Close HTML
  html << "    </table>\n"
       << "    <footer>\n"
       << "      webserv/1.0 · Autoindex\n"
       << "    </footer>\n"
       << "  </div>\n"
       << "</body>\n"
       << "</html>";

  return html.str();
}

/**
 * @brief Escapes HTML special characters
 *
 * Prevents XSS attacks by converting:
 * - & → &amp;
 * - < → &lt;
 * - > → &gt;
 * - " → &quot;
 * - ' → &#39;
 *
 * @param input Raw string
 * @return HTML-safe string
 */
std::string escapeHtml(const std::string &input) {
  std::string output;
  output.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    switch (input[i]) {
    case '&':
      output.append("&amp;");
      break;
    case '<':
      output.append("&lt;");
      break;
    case '>':
      output.append("&gt;");
      break;
    case '"':
      output.append("&quot;");
      break;
    case '\'':
      output.append("&#39;");
      break;
    default:
      output.push_back(input[i]);
      break;
    }
  }
  return output;
}

/**
 * @brief URL-encodes a string for use in href attributes
 *
 * Converts special characters to %XX format per RFC 3986.
 * Safe characters (alphanumeric, -, _, ., ~) pass through.
 * Spaces become %20.
 *
 * @param input Raw filename or path segment
 * @return URL-encoded string
 *
 * @note Example: "My File#.txt" → "My%20File%23.txt"
 */
std::string urlEncode(const std::string &input) {
  std::ostringstream encoded;
  for (size_t i = 0; i < input.size(); ++i) {
    unsigned char c = input[i];

    // Safe characters (RFC 3986)
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
    } else if (c == ' ') {
      encoded << "%20";
    } else {
      // Encode as %XX (hex)
      encoded << '%' << std::hex << std::uppercase << (int)(c >> 4)
              << (int)(c & 0x0F);
    }
  }
  return encoded.str();
}

} // namespace Autoindex
