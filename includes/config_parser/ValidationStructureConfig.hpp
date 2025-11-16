#ifndef VALIDATIONCONFIGFILE_HPP
#define VALIDATIONCONFIGFILE_HPP

#include <string>

bool isEmptyBraceOrSemicolonLine(const std::string &trimmedLine, int &lineCont, const std::string &filePath);

int contOpenKeys(const std::string &trimmedLine, int &lineCont, int &contOpenKey);

int contCloseKeys(const std::string &trimmedLine, int &lineCont, int &contCloseKey);

void processConfigLine(const std::string &trimmedLine, int &lineCont, int &contOpenKey,
                       int &contCloseKey, int &firstOpenKey, int &lastCloseKey);

bool resultProcesConfigLine(int contOpenKey, int contCloseKey, int firstOpenKey,
                            int lastCloseKey, const std::string &filePath);

bool firstNonAlNumChar(const std::string &trimmedLine, int &lineCont, const std::string &filePath);

bool validationStructureConfigFile(const std::string &filePath);

#endif