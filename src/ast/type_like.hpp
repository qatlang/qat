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

	useit ir::Type* emit(EmitCtx* ctx) const;
};

} // namespace qat::ast

#endif
