#ifndef QAT_AST_TYPE_LIKE_HPP
#define QAT_AST_TYPE_LIKE_HPP

#include "./emit_ctx.hpp"

namespace qat::ast {

struct TypeLike {
	bool  isType;
	void* data;

	useit static TypeLike from_type(Type* type) { return TypeLike{.isType = true, .data = (void*)type}; }

	useit static TypeLike from_prerun(PrerunExpression* preExp) {
		return TypeLike{.isType = false, .data = (void*)preExp};
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx);

	useit ir::Type* emit(EmitCtx* ctx) const;

	useit operator bool() const { return data != nullptr; }

	useit FileRange get_range() const;

	useit String to_string() const;

	useit JsonValue to_json_value() const;
};

} // namespace qat::ast

#endif
