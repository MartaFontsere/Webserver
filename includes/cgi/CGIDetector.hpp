#ifndef CGIDETECTOR_HPP
#define CGIDETECTOR_HPP

#include <string>
#include <vector>

/**
 * @brief CGI detection utilities - identifies CGI requests by extension
 */
class CGIDetector {
private:
  static std::string getExtension(const std::string &path);

public:
  /** @brief Check if URI matches configured CGI extensions */
  static bool isCGIRequest(const std::string &uri,
                           const std::vector<std::string> &cgiExts);

  /** @brief Find executable for script based on extension */
  static std::string getCGIExecutable(const std::string &scriptPath,
                                      const std::vector<std::string> &cgiPaths,
                                      const std::vector<std::string> &cgiExsts);

  static std::string resolveScriptPath(const std::string &uri,
                                       const std::string &root);

  static std::string removeQueryString(const std::string &uri);
};

#endif