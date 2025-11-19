#include "../../includes/config_parser/DirectiveMetadata.hpp"
#include <cstring>

// ========== TABLA DE REGLAS ==========

const DirectiveRule DirectiveMetadata::rules[] = {

    {"dummy", CTX_MAIN, 0, 0, ARG_STR, false}};

const size_t DirectiveMetadata::rulesCount = sizeof(rules) / sizeof(rules[0]);

// ========== IMPLEMENTACIONES ==========

const DirectiveRule *DirectiveMetadata::getRule(const std::string &directiveName)
{
    (void)directiveName;
    return NULL;
}

bool DirectiveMetadata::isValidInContext(const std::string &directive, Context ctx)
{
    (void)directive;
    (void)ctx;
    return false;
}

bool DirectiveMetadata::validateArguments(const std::string &directive,
                                          const std::vector<std::string> &args)
{
    (void)directive;
    (void)args;
    return false;
}