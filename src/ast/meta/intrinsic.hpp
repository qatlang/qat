#ifndef QAT_AST_META_GET_INTRINSIC_HPP
#define QAT_AST_META_GET_INTRINSIC_HPP

#include "../expression.hpp"

namespace qat::ast {

// NOTE - !!! Update the standard library if this is updated
enum class IntrinsicID : u32 {
	matrix_multiply = 0,
	matrix_transpose,
};

class MetaIntrinsic final : public Expression {
	Vec<PrerunExpression*> args;

  public:
	MetaIntrinsic(Vec<PrerunExpression*> _types, FileRangePtr _fileRange) : Expression(_fileRange), args(_types) {}

	useit static MetaIntrinsic* create(Vec<PrerunExpression*> _args, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(MetaIntrinsic), _args, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto arg : args) {
			UPDATE_DEPS(arg);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::META_INTRINSIC; }
};

} // namespace qat::ast

#endif
