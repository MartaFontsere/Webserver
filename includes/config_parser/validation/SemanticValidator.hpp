#ifndef SEMANTICVALIDATOR_HPP
#define SEMANTICVALIDATOR_HPP

#include "../parser/BlockParser.hpp"
#include "DirectiveMetadata.hpp"
#include <string>
#include <vector>

/**
 * @brief Semantic validator - checks directive context and argument validity
 */
class SemanticValidator {
private:
  std::vector<std::string> _errors;
  std::vector<std::string> _warnings;

  void validateBlock(const BlockParser &block, Context ctx);
  void validateDirective(const DirectiveToken &directive, Context ctx);
  Context getBlockContext(const std::string &blockName) const;

public:
  SemanticValidator();

  /** @brief Validate entire config tree, returns false on error */
  bool validate(const BlockParser &rootParser);
  const std::vector<std::string> &getErrors() const;
  const std::vector<std::string> &getWarnings() const;
  bool hasErrors() const;
  void clear();
  void printReport() const;
};

#endif