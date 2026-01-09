#ifndef QAT_AST_EXPRESSIONS_IS_FLAG_VARIANT_HPP
#define QAT_AST_EXPRESSIONS_IS_FLAG_VARIANT_HPP

#include "../expression.hpp"

namespace qat::ast {

enum class FlagVariantKind {
	NONE,
	DEFAULT,
	VARIANTS,
};

class IsFlagVariant : public Expression {
	Expression*     candidate;
	FlagVariantKind kind;
	Vec<Identifier> variants;

  public:
	IsFlagVariant(Expression* _candidate, FlagVariantKind _kind, Vec<Identifier> _variants, FileRangePtr _fileRange)
	    : Expression(_fileRange), candidate(_candidate), kind(_kind), variants(std::move(_variants)) {}

	useit static IsFlagVariant* create(Expression* candidate, FlagVariantKind kind, Vec<Identifier> variants,
	                                   FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(IsFlagVariant), candidate, kind, std::move(variants), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
	}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::IS_FLAG_VARIANT; }
};

} // namespace qat::ast

#endif
