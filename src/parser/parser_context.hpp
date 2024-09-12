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

  useit bool has_typed_generic(const String& name) const;
  useit bool has_prerun_generic(const String& name) const;
  void       add_abstract_generic(ast::GenericAbstractType* type);
  void       remove_named_generic_abstract(const String& name);
  useit ast::TypedGenericAbstract* get_typed_generic(const String& name);
  useit ast::PrerunGenericAbstract* get_prerun_generic(const String& name);

private:
  // All generic abstracts available in the current scope
  Deque<ast::GenericAbstractType*> generics;
};

} // namespace qat::parser

#endif
