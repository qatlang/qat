#ifndef QAT_PARSER_PARSER_CONTEXT_HPP
#define QAT_PARSER_PARSER_CONTEXT_HPP

#include "../ast/types/qat_type.hpp"
#include "../ast/types/typed_generic.hpp"
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

  useit bool hasTypedGeneric(const String& name) const;
  useit bool hasConstGeneric(const String& name) const;
  void       addAbstractGeneric(ast::GenericAbstractType* type);
  void       removeNamedGenericAbstract(const String& name);
  useit ast::TypedGeneric* getTypedGeneric(const String& name);
  useit ast::PrerunGeneric* getConstGeneric(const String& name);

private:
  // All generic abstracts available in the current scope
  Deque<ast::GenericAbstractType*> generics;
};

} // namespace qat::parser

#endif