#ifndef VALIDATIONCONFIGFILE_HPP
#define VALIDATIONCONFIGFILE_HPP

#include <string>

void isEmptyBraceOrSemicolonLine(const std::string &trimmedLine, int &lineCont, const std::string &filePath);

void checkEmptyBraceOrSemicolon(const std::string &trimmedLine, int lineCont, const std::string &filePath,
                                std::vector<std::string> &errors);

int contOpenKeys(const std::string &trimmedLine, int &lineCont, int &contOpenKey);

int contCloseKeys(const std::string &trimmedLine, int &lineCont, int &contCloseKey);

void processConfigLine(const std::string &trimmedLine, int &lineCont, int &contOpenKey,
                    int &contCloseKey, int &firstOpenKey, int &lastCloseKey);

void resultProcesConfigLine(int contOpenKey, int contCloseKey, int firstOpenKey,
                            int lastCloseKey, const std::string &filePath);

void firstNonAlNumChar(const std::string &trimmedLine, int &lineCont, const std::string &filePath);

bool validateStructure(const std::string &filePath, std::vector<std::string> &errors);

void validationStructureConfigFile(const std::string &filePath);

#endif