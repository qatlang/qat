#ifndef QAT_PARSER_PARSER_CONTEXT_HPP
#define QAT_PARSER_PARSER_CONTEXT_HPP

#include "../ast/types/qat_type.hpp"
#include "../utils/helpers.hpp"
#include <deque>
#include <map>
#include <string>

namespace qat::parser {

/**
 *  ParserContext will be shared between different types of parser
 * functions for managing shared entities/definitions within the scope
 *
 */
class ParserContext {
public:
  ParserContext();

  useit bool hasTemplate(const String& name) const;
  void       addTemplate(ast::GenericAbstractType* templateType);
  void       removeTemplate(const String& name);
  useit ast::GenericAbstractType* getTemplate(const String& name);
  useit ast::GenericAbstractType* duplicateTemplate(const String& name, bool isVariable, FileRange fileRange);

private:
  // All template typenames available in the current scope
  Deque<ast::GenericAbstractType*> templates;
};

} // namespace qat::parser

#endif