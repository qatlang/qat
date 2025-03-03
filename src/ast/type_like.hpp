#ifndef QAT_AST_TYPE_LIKE_HPP
#define QAT_AST_TYPE_LIKE_HPP

#include "./emit_ctx.hpp"

namespace qat::ast {

class Expression;

enum class TypeLikeKind { TYPE, PRERUN, EXPRESSION };

struct TypeLike {
	TypeLikeKind kind;
	void*        data;

	useit static TypeLike from_type(Type* type) { return TypeLike{.kind = TypeLikeKind::TYPE, .data = (void*)type}; }

	useit static TypeLike from_prerun(PrerunExpression* preExp) {
		return TypeLike{.kind = TypeLikeKind::PRERUN, .data = (void*)preExp};
	}

	useit static TypeLike from_expression(Expression* exp) {
		return TypeLike{.kind = TypeLikeKind::EXPRESSION, .data = (void*)exp};
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx);

	useit ir::Type* emit(EmitCtx* ctx) const;

	useit operator bool() const { return data != nullptr; }

	useit FileRange get_range() const;

	useit String to_string() const;

	useit operator JsonValue() const;
};

} // namespace qat::ast

#endif
