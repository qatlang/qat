#ifndef QAT_AST_TYPE_LIKE_HPP
#define QAT_AST_TYPE_LIKE_HPP

#include "./emit_ctx.hpp"

namespace qat::ast {

class Expression;
class Type;

enum class TypeLikeKind { TYPE, PRERUN, EXPRESSION };

class TypeLike {
	TypeLikeKind kind;
	void*        data;

	TypeLike(TypeLikeKind _kind, void* _data) : kind(_kind), data(_data) {}

  public:
	TypeLike() : kind(TypeLikeKind::TYPE), data(nullptr) {}

	static TypeLike from_type(Type* type) { return TypeLike(TypeLikeKind::TYPE, (void*)type); }

	static TypeLike from_prerun(PrerunExpression* preExp) { return TypeLike(TypeLikeKind::PRERUN, (void*)preExp); }

	static TypeLike from_expression(Expression* exp) { return TypeLike(TypeLikeKind::EXPRESSION, (void*)exp); }

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx);

	ir::Type* emit(EmitCtx* ctx) const;

	operator bool() const { return data != nullptr; }

	FileRangePtr get_range() const;

	String to_string() const;
};

} // namespace qat::ast

#endif
