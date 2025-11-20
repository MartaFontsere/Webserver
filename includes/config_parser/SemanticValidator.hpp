#ifndef SEMANTICVALIDATOR_HPP
#define SEMANTICVALIDATOR_HPP

#include <string>
#include <vector>
#include "BlockParser.hpp"          
#include "DirectiveMetadata.hpp"

class SemanticValidator
{
private:
    std::vector<std::string> _errors;
    std::vector<std::string> _warnings;


    void validateBlock(const BlockParser &block, Context ctx);
    void validateDirective(const DirectiveToken &directive, Context ctx);
    Context getBlockContext(const std::string &blockName) const;

public:
    SemanticValidator();

    bool validate(const BlockParser &rootParser);
    const std::vector<std::string>& getErrors() const;
    const std::vector<std::string>& getWarnings() const;
    bool hasErrors() const;
    void clear();
    void printReport() const;
};

#endif