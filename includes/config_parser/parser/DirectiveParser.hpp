#ifndef DIRECTIVEPARSER_HPP
#define DIRECTIVEPARSER_HPP

#include <string>
#include <vector>

/** @brief Token representing a parsed directive (name + values) */
struct DirectiveToken {
  std::string name;
  std::vector<std::string> values;
  int lineNumber;

  DirectiveToken() : lineNumber(0) {}
};

/**
 * @brief Directive parser - extracts name and values from config lines
 */
class DirectiveParser {
private:
  std::vector<DirectiveToken> _directives;

public:
  DirectiveParser();
  DirectiveParser(const DirectiveParser &other);
  DirectiveParser &operator=(const DirectiveParser &other);
  ~DirectiveParser();

  bool parseDirective(const std::vector<std::string> &tokens,
                      int lineNumber = 0);
  void printDirectives() const;
  const std::vector<DirectiveToken> &getDirectives() const;
};

#endif