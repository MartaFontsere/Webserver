#ifndef CGIDETECTOR_HPP
#define CGIDETECTOR_HPP

#include <string>
#include <vector>

class CGIDetector
{
private:
    static std::string getExtension(const std::string &path);

public:
    static bool isCGIRequest(const std::string &uri,
                             const std::vector<std::string> &cgiExts);

    static std::string getCGIExecutable(const std::string &scriptPath,
                                        const std::vector<std::string> &cgiPaths, const std::vector<std::string> &cgiExsts);

    static std::string resolveScriptPath(const std::string &uri,
                                         const std::string &root);

    static std::string removeQueryString(const std::string &uri);
};

#endif