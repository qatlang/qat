#ifndef QAT_AST_EXPRESSIONS_IS_VARIANT_HPP
#define QAT_AST_EXPRESSIONS_IS_VARIANT_HPP

#include "../expression.hpp"

namespace qat::ast {

enum class IsVariantKind : u8 {
	POINTER_NULL,
	RESULT_OK,
	RESULT_ERROR,
	MAYBE_VALUE,
	NONE,
	BOOL_TRUE,
	BOOL_FALSE,
	VARIANT_NAME,
	VARIABILITY,
};

useit inline String is_variant_kind_to_string(IsVariantKind kind) {
	switch (kind) {
		case IsVariantKind::POINTER_NULL:
			return "null";
		case IsVariantKind::RESULT_OK:
			return "ok";
		case IsVariantKind::RESULT_ERROR:
			return "error";
		case IsVariantKind::MAYBE_VALUE:
			return "is";
		case IsVariantKind::NONE:
			return "none";
		case IsVariantKind::BOOL_TRUE:
			return "true";
		case IsVariantKind::BOOL_FALSE:
			return "false";
		case IsVariantKind::VARIANT_NAME:
			return "name";
		case IsVariantKind::VARIABILITY:
			return "var";
	}
}

class IsVariant final : public Expression {
	Expression*       expression;
	IsVariantKind     kind;
	Maybe<Identifier> name;

  public:
	IsVariant(Expression* _expression, IsVariantKind _kind, Maybe<Identifier> _name, FileRangePtr _fileRange)
	    : Expression(_fileRange), expression(_expression), kind(_kind), name(std::move(_name)) {}

	useit static IsVariant* create(Expression* expression, IsVariantKind kind, Maybe<Identifier> name,
	                               FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(IsVariant), expression, kind, std::move(name), fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::IS_VARIANT; }
};

} // namespace qat::ast

#endif
