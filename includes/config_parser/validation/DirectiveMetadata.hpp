#ifndef DIRECTIVEMETADATA_HPP
#define DIRECTIVEMETADATA_HPP

#include <string>
#include <vector>

#define MAX_ARGS 5

/** @brief Configuration context flags (bitwise combinable) */
enum Context {
  CTX_MAIN = 1,
  CTX_EVENTS = 2,
  CTX_HTTP = 4,
  CTX_SERVER = 8,
  CTX_LOCATION = 16
};

/** @brief Argument type for validation */
enum ArgumentType {
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

/** @brief Directive validation rule */
struct DirectiveRule {
  const char *name;
  int allowedContexts;
  int minArgs;
  int maxArgs;
  ArgumentType argType[MAX_ARGS];
  bool unique;
};

/**
 * @brief Directive metadata - validation rules for all nginx directives
 */
class DirectiveMetadata {
private:
  static const DirectiveRule rules[];
  static const size_t rulesCount;

  static bool validateArgumentTypes(const DirectiveRule *rule,
                                    const std::vector<std::string> &args);

public:
  static const DirectiveRule *getRule(const std::string &directiveName);
  static bool isValidInContext(const std::string &directive, Context ctx);
  static bool validateArguments(const std::string &directive,
                                const std::vector<std::string> &args);
};

#endif