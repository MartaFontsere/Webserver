#include "../../../includes/config_parser/validation/DirectiveMetadata.hpp"
#include "../../../includes/config_parser/validation/ValueValidator.hpp"
#include <cstring>

const DirectiveRule DirectiveMetadata::rules[] = {
    // PENDIENTE VALORAR EN EQUIPO SI SE QUEDA ASI O AJUSTAMOS "PARAMETROS"
    // DESPUES DE IMPLEMENTACION REAL

    // SERVER CONTEXT
    {"listen",        CTX_SERVER, 1, -1, {ARG_PORT, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, false},
    {"server_name",   CTX_SERVER, 1, -1, {ARG_HOST, ARG_HOST, ARG_HOST, ARG_HOST, ARG_HOST}, true},
    {"host",          CTX_SERVER, 1,  1, {ARG_IP, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},

    // HTTP | SERVER | LOCATION
    {"root",          CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, {ARG_PATH, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},
    {"index",         CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, -1, {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},
    {"error_page",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 2, -1, {ARG_HTTP, ARG_PATH, ARG_HTTP, ARG_HTTP, ARG_HTTP}, false},
    {"autoindex",     CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, {ARG_BOOL, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},

    // SERVER
    {"location",      CTX_SERVER, 1, 1, {ARG_PATTERN, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, false},

    // LOCATION ONLY
    {"allow_methods", CTX_LOCATION, 1, -1, {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},
    {"proxy_pass",    CTX_LOCATION, 1,  1, {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},
    {"cgi_path",      CTX_LOCATION, 1, -1, {ARG_PATH, ARG_PATH, ARG_PATH, ARG_PATH, ARG_PATH}, true},
    {"cgi_ext",       CTX_LOCATION, 1, -1, {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true},

    // SERVER | LOCATION
    {"return",        CTX_SERVER|CTX_LOCATION, 1, 2, {ARG_HTTP, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, false},
    {"rewrite",       CTX_SERVER|CTX_LOCATION, 2, 3, {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, false},

    // HTTP | SERVER
    {"client_max_body_size", CTX_HTTP|CTX_SERVER, 1, 1, {ARG_NUMBER, ARG_STR, ARG_STR, ARG_STR, ARG_STR}, true}
 
    // ========== BONUS: Cookies & Sessions (comentadas para más adelante) poner los 5 arg type, ahora solo hay uno, riesgo de crashh==========
    // Session management
    // {"session_timeout",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_NUMBER, true}, //Tiempo de vida de la sesión en segundos.
    // {"session_name",       CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_STR,    true}, //Nombre del identificador de sesión (cookie).
    // {"session_path",       CTX_HTTP|CTX_SERVER,              1, 1, ARG_PATH,   true}, //Directorio para almacenar datos de sesión.

    // Cookie configuration
    // {"cookie_domain",      CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_HOST,   true}, //Define el dominio para las cookies (ej: .example.com para subdominios).
    // {"cookie_path",        CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_PATH,   true}, //Ruta donde la cookie es válida.
    // {"cookie_max_age",     CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_NUMBER, true}, //Tiempo de vida de la cookie en segundos.
    // {"cookie_secure",      CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_BOOL,   true}, //Cookie solo se envía por conexiones seguras (HTTPS).
    // {"cookie_httponly",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_BOOL,   true}, //La cookie NO es accesible desde JavaScript (seguridad).
    // {"cookie_samesite",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_STR,    true} //Protección CSRF (Cross-Site Request Forgery).
};

const size_t DirectiveMetadata::rulesCount = sizeof(rules) / sizeof(rules[0]);

const DirectiveRule *DirectiveMetadata::getRule(const std::string &directiveName)
{
    for (size_t i = 0; i < rulesCount; ++i)
    {
        if (directiveName == rules[i].name)
            return &rules[i];
    }
    return NULL;
}

bool DirectiveMetadata::isValidInContext(const std::string &directive, Context ctx)
{
    const DirectiveRule *rule = getRule(directive);

    if (rule == NULL)
        return false;
    if ((rule->allowedContexts & ctx) != 0)
        return true;
    return false;
}

bool DirectiveMetadata::validateArgumentTypes(const DirectiveRule *rule,
                                        const std::vector<std::string> &args)
{
    for (size_t i = 0; i < args.size(); ++i)
    {
        size_t typeIndex;
        if (i < MAX_ARGS)
            typeIndex = i;
        else
            typeIndex = MAX_ARGS - 1;
        ArgumentType type = rule->argType[typeIndex];

        switch (type)
        {
            case ARG_HTTP:
                return isValidHttpCode(args[0]);
            case ARG_NUMBER:
                return isValidNumber(args[0]);
            case ARG_PORT:
                return isValidPort(args[0]);
            case ARG_BOOL:
                return isValidBool(args[0]);
            case ARG_PATH:
                return isValidPath(args[0]);
            case ARG_IP:
                return isValidIP(args[0]);
            case ARG_HOST:
                return isValidHost(args[0]);
            case ARG_PATTERN:
                return isValidPattern(args[0]);
            case ARG_STR:
            default:
                return true;
        }
    }
    return true;
}

bool DirectiveMetadata::validateArguments(const std::string &directive,
                                        const std::vector<std::string> &args)
{
    const DirectiveRule *rule = getRule(directive);
    if (rule == NULL)
        return false;

    int argCount = static_cast<int>(args.size());

    if (argCount < rule->minArgs)
        return false;

    if (rule->maxArgs != -1 && argCount > rule->maxArgs)
        return false;

    for (size_t i = 0; i < args.size(); ++i)
    {
        if (args[i].empty())
            return false;
    }
    return validateArgumentTypes(rule, args);
}