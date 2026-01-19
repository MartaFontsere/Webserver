#ifndef VALIDATIONCONFIGFILE_HPP
#define VALIDATIONCONFIGFILE_HPP

#include <string>
#include <vector>

/** @brief Structural validation utilities for config files */
void checkEmptyBraceOrSemicolon(const std::string &trimmedLine, int lineCont,
                                const std::string &filePath,
                                std::vector<std::string> &errors);

void checkInvalidCharacters(const std::string &trimmedLine, int lineCont,
                            std::vector<std::string> &errors);

void checkBraceBalance(int contOpenKey, int contCloseKey, int firstOpenKey,
                       int lastCloseKey, const std::string &filePath,
                       std::vector<std::string> &errors);

int contOpenKeys(const std::string &trimmedLine, int &lineCont,
                 int &contOpenKey);

int contCloseKeys(const std::string &trimmedLine, int &lineCont,
                  int &contCloseKey);

void processConfigLine(const std::string &trimmedLine, int &lineCont,
                       int &contOpenKey, int &contCloseKey, int &firstOpenKey,
                       int &lastCloseKey);

/** @brief Validate config file structure (braces, semicolons) */
bool validateStructure(const std::string &filePath,
                       std::vector<std::string> &errors);

#endif