#ifndef VALIDATIONCONFIGFILE_HPP
#define VALIDATIONCONFIGFILE_HPP

#include <string>

bool isEmptyBraceOrSemicolonLine(std::string line, int *lineCont, const std::string &filePath);

int contOpenKeys(std::string line, int lineCont, int *contOpenKey);

int contCloseKeys(std::string line, int lineCont, int *contCloseKey);

void processConfigLine(const std::string &line, int *lineCont, int *contOpenKey,
                       int *contCloseKey, int *firstOpenKey, int *lastCloseKey);

bool resultProcesConfigLine(int contOpenKey, int contCloseKey, int firstOpenKey,
                            int lastCloseKey, const std::string &filePath);

bool incorrectLineTermination(const std::string &line, int *lineCont, const std::string &filePath);

bool firstNonAlNumChar(const std::string &line, int *lineCont, const std::string &filePath);

bool validationConfigFile(const std::string &filePath);

#endif