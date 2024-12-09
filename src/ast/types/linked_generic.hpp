#ifndef QAT_AST_LINKED_GENERIC_HPP
#define QAT_AST_LINKED_GENERIC_HPP

#include "generic_abstract.hpp"
#include "qat_type.hpp"

namespace qat::ast {
class LinkedGeneric final : public Type {
	ast::GenericAbstractType* genAbs;

  public:
	LinkedGeneric(ast::GenericAbstractType* _genAbs, FileRange _range) : Type(_range), genAbs(_genAbs) {}

	useit static LinkedGeneric* create(ast::GenericAbstractType* _genAbs, FileRange _range) {
		return std::construct_at(OwnNormal(LinkedGeneric), _genAbs, _range);
	}

	useit ir::Type*   emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final;
	useit Json        to_json() const final;
	useit String      to_string() const final;
};

} // namespace qat::ast

#endif
