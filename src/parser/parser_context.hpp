#ifndef QAT_PARSER_PARSER_CONTEXT_HPP
#define QAT_PARSER_PARSER_CONTEXT_HPP

#include "../ast/types/qat_type.hpp"
#include "../ast/types/typed_generic.hpp"

#include <helpers/deque.hpp>
#include <helpers/string.hpp>

namespace qat::parser {

/**
 *  ParserContext will be shared between different types of parser
 * functions for managing shared entities/definitions within the scope
 *
 */
class ParserContext {
  public:
	ParserContext();

	bool                        has_typed_generic(String const& name) const;
	bool                        has_prerun_generic(String const& name) const;
	void                        add_abstract_generic(ast::GenericAbstractType* type);
	void                        remove_named_generic_abstract(String const& name);
	ast::TypedGenericAbstract*  get_typed_generic(String const& name);
	ast::PrerunGenericAbstract* get_prerun_generic(String const& name);

  private:
	// All generic abstracts available in the current scope
	Deque<ast::GenericAbstractType*> generics;
};

} // namespace qat::parser

#endif
