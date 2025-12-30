#include "../../../includes/config_parser/validation/DirectiveMetadata.hpp"
#include "../../../includes/config_parser/validation/ValueValidator.hpp"
#include <cstring>

/**
 * @brief Complete table of nginx-style directive validation rules
 *
 * Each rule defines:
 * - name: Directive identifier (e.g., "listen", "root")
 * - allowedContexts: Bitwise OR of valid contexts (CTX_HTTP | CTX_SERVER, etc.)
 * - minArgs: Minimum number of arguments required
 * - maxArgs: Maximum arguments allowed (-1 = unlimited)
 * - argType[MAX_ARGS]: Expected type for each argument position
 * - canRepeat: Whether the directive can appear multiple times
 *
 * Argument types are validated per position:
 * - args[0] validated against argType[0]
 * - args[1] validated against argType[1]
 * - args[5+] validated against argType[4] (last type repeats)
 *
 * Context flags (bitwise):
 * - CTX_MAIN = 1, CTX_HTTP = 4, CTX_SERVER = 8, CTX_LOCATION = 16, CTX_EVENTS =
 * 2
 */
const DirectiveRule DirectiveMetadata::rules[] = {
    // NOTE: Review argument types with team after real implementation testingL

    // SERVER CONTEXT
    {"listen",
     CTX_SERVER,
     1,
     -1,
     {ARG_PORT, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     false},
    {"server_name",
     CTX_SERVER,
     1,
     -1,
     {ARG_HOST, ARG_HOST, ARG_HOST, ARG_HOST, ARG_HOST},
     true},
    {"host",
     CTX_SERVER,
     1,
     1,
     {ARG_IP, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},

    // HTTP | SERVER | LOCATION
    {"root",
     CTX_HTTP | CTX_SERVER | CTX_LOCATION,
     1,
     1,
     {ARG_PATH, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},
    {"index",
     CTX_HTTP | CTX_SERVER | CTX_LOCATION,
     1,
     -1,
     {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},
    {"error_page",
     CTX_HTTP | CTX_SERVER | CTX_LOCATION,
     2,
     -1,
     {ARG_HTTP, ARG_PATH, ARG_HTTP, ARG_HTTP, ARG_HTTP},
     false},
    {"autoindex",
     CTX_HTTP | CTX_SERVER | CTX_LOCATION,
     1,
     1,
     {ARG_BOOL, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},

    // SERVER
    {"location",
     CTX_SERVER,
     1,
     1,
     {ARG_PATTERN, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     false},

    // LOCATION ONLY
    {"allow_methods",
     CTX_LOCATION,
     1,
     -1,
     {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},
    {"proxy_pass",
     CTX_LOCATION,
     1,
     1,
     {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},
    {"cgi_path",
     CTX_LOCATION,
     1,
     -1,
     {ARG_PATH, ARG_PATH, ARG_PATH, ARG_PATH, ARG_PATH},
     true},
    {"cgi_ext",
     CTX_LOCATION,
     1,
     -1,
     {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},
    {"alias",
     CTX_LOCATION,
     1,
     1,
     {ARG_PATH, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},
    {"upload_path",
     CTX_LOCATION,
     1,
     1,
     {ARG_PATH, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true},

    // SERVER | LOCATION
    {"return",
     CTX_SERVER | CTX_LOCATION,
     1,
     2,
     {ARG_HTTP, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     false},
    {"rewrite",
     CTX_SERVER | CTX_LOCATION,
     2,
     3,
     {ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     false},

    // HTTP | SERVER
    {"client_max_body_size",
     CTX_HTTP | CTX_SERVER | CTX_LOCATION,
     1,
     1,
     {ARG_NUMBER, ARG_STR, ARG_STR, ARG_STR, ARG_STR},
     true}

    // ========== BONUS: Cookies & Sessions (comentadas para más adelante) poner
    // los 5 arg type, ahora solo hay uno, riesgo de crashh==========
    // Session management
    // {"session_timeout",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1,
    // ARG_NUMBER, true}, //Tiempo de vida de la sesión en segundos.
    // {"session_name",       CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_STR,
    // true}, //Nombre del identificador de sesión (cookie).
    // {"session_path",       CTX_HTTP|CTX_SERVER,              1, 1, ARG_PATH,
    // true}, //Directorio para almacenar datos de sesión.

    // Cookie configuration
    // {"cookie_domain",      CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_HOST,
    // true}, //Define el dominio para las cookies (ej: .example.com para
    // subdominios).
    // {"cookie_path",        CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_PATH,
    // true}, //Ruta donde la cookie es válida.
    // {"cookie_max_age",     CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1,
    // ARG_NUMBER, true}, //Tiempo de vida de la cookie en segundos.
    // {"cookie_secure",      CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_BOOL,
    // true}, //Cookie solo se envía por conexiones seguras (HTTPS).
    // {"cookie_httponly",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_BOOL,
    // true}, //La cookie NO es accesible desde JavaScript (seguridad).
    // {"cookie_samesite",    CTX_HTTP|CTX_SERVER|CTX_LOCATION, 1, 1, ARG_STR,
    // true} //Protección CSRF (Cross-Site Request Forgery).
};

/**
 * @brief Total number of directive rules in the table
 *
 * Calculated at compile time using sizeof. Used for iterating through
 * the rules array without hardcoding the count.
 */
const size_t DirectiveMetadata::rulesCount = sizeof(rules) / sizeof(rules[0]);

/**
 * @brief Searches for a directive rule by name
 *
 * Performs a linear search through the rules table to find a directive
 * matching the given name. Case-sensitive comparison.
 *
 * @param directiveName Name of the directive to find (e.g., "listen", "root")
 * @return Pointer to DirectiveRule if found, NULL if not found
 */
const DirectiveRule *
DirectiveMetadata::getRule(const std::string &directiveName) {
  for (size_t i = 0; i < rulesCount; ++i) {
    if (directiveName == rules[i].name)
      return &rules[i];
  }
  return NULL;
}

/**
 * @brief Validates if a directive is allowed in a specific context
 *
 * Uses bitwise AND to check if the directive's allowed contexts include
 * the specified context. This allows directives to be valid in multiple
 * contexts (e.g., "root" in HTTP, SERVER, and LOCATION).
 *
 * Example:
 *   rule->allowedContexts = CTX_HTTP | CTX_SERVER (12 = 0b1100)
 *   ctx = CTX_SERVER (8 = 0b1000)
 *   (12 & 8) = 8 ≠ 0 → true (valid)
 *
 * @param directive Name of the directive to check
 * @param ctx Context to validate against (CTX_MAIN, CTX_HTTP, CTX_SERVER, etc.)
 * @return true if directive is allowed in the context, false otherwise
 */
bool DirectiveMetadata::isValidInContext(const std::string &directive,
                                         Context ctx) {
  const DirectiveRule *rule = getRule(directive);

  if (rule == NULL)
    return false;
  if ((rule->allowedContexts & ctx) != 0)
    return true;
  return false;
}

/**
 * @brief Validates the types of all arguments (LEVEL 3 validation)
 *
 * Checks each argument against the expected type defined in the rule.
 * For arguments beyond MAX_ARGS (5), uses the last type in argType array.
 *
 * Type validation uses ValueValidator functions:
 * - ARG_HTTP → isValidHttpCode()
 * - ARG_PORT → isValidPort()
 * - ARG_BOOL → isValidBool()
 * - ARG_PATH → isValidPath()
 * - ARG_IP → isValidIP()
 * - ARG_HOST → isValidHost()
 * - ARG_PATTERN → isValidPattern()
 * - ARG_NUMBER → isValidNumber()
 * - ARG_STR → always true (accepts any string)
 *
 * @param rule Pointer to the directive rule containing type definitions
 * @param args Vector of argument strings to validate
 * @return true if all arguments match expected types, false otherwise
 */
bool DirectiveMetadata::validateArgumentTypes(
    const DirectiveRule *rule, const std::vector<std::string> &args) {
  for (size_t i = 0; i < args.size(); ++i) {
    size_t typeIndex;
    if (i < MAX_ARGS)
      typeIndex = i;
    else
      typeIndex = MAX_ARGS - 1;
    ArgumentType type = rule->argType[typeIndex];

    switch (type) {
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

/**
 * @brief Complete argument validation (LEVEL 1 + LEVEL 2 + LEVEL 3)
 *
 * Performs three levels of validation:
 * - LEVEL 1: Argument count (min/max validation)
 * - LEVEL 2: First argument type validation
 * - LEVEL 3: ALL arguments type validation
 *
 * Also verifies that no argument is empty (empty strings not allowed).
 *
 * Validation process:
 * 1. Get directive rule
 * 2. Check argument count (minArgs ≤ count ≤ maxArgs)
 * 3. Verify no empty arguments
 * 4. Validate all argument types
 *
 * @param directive Name of the directive being validated
 * @param args Vector of argument strings
 * @return true if all validations pass, false if any fails
 */
bool DirectiveMetadata::validateArguments(
    const std::string &directive, const std::vector<std::string> &args) {
  const DirectiveRule *rule = getRule(directive);
  if (rule == NULL)
    return false;

  int argCount = static_cast<int>(args.size());
  // LEVEL 1: Argument count validation
  if (argCount < rule->minArgs)
    return false;

  if (rule->maxArgs != -1 && argCount > rule->maxArgs)
    return false;
  // Verify no empty arguments
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i].empty())
      return false;
  }
  // LEVEL 2 + LEVEL 3: Type validation
  return validateArgumentTypes(rule, args);
}