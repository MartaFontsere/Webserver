#ifndef DIRECTIVEMETADATA_HPP
#define DIRECTIVEMETADATA_HPP

#include <string>
#include <vector>

// ========== 1. CONTEXTOS ==========
enum Context
{
    CTX_MAIN = 1,     // 0b00001
    CTX_EVENTS = 2,   // 0b00010
    CTX_HTTP = 4,     // 0b00100
    CTX_SERVER = 8,   // 0b01000
    CTX_LOCATION = 16 // 0b10000
};

// ========== 2. TIPOS DE ARGUMENTOS ==========
enum ArgumentType
{
    ARG_NUMBER,
    ARG_PORT,
    ARG_STR,
    ARG_PATH,
    ARG_HOST,
    ARG_IP,
    ARG_HTTP,
    ARG_BOOL,
    ARG_PATTERN
};

// ========== 3. ESTRUCTURA DIRECTIVERULE ==========
struct DirectiveRule
{
    const char *name;     // Nombre de la directiva
    int allowedContexts;  // Bitfield de contextos
    int minArgs;          // Mínimo argumentos
    int maxArgs;          // Máximo argumentos (-1 = ilimitado)
    ArgumentType argType; // Tipo de argumento
    bool unique;          // ¿Solo una por contexto?
};

// ========== 4. CLASE DIRECTIVEMETADATA ==========
class DirectiveMetadata
{
private:
    static const DirectiveRule rules[];
    static const size_t rulesCount;

public:
    static const DirectiveRule *getRule(const std::string &directiveName);
    static bool isValidInContext(const std::string &directive, Context ctx);
    static bool validateArguments(const std::string &directive, const std::vector<std::string> &args);
};

#endif