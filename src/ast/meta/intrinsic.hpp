#ifndef QAT_AST_META_GET_INTRINSIC_HPP
#define QAT_AST_META_GET_INTRINSIC_HPP

#include "../expression.hpp"

namespace qat::ast {

// NOTE - !!! Update the standard library if this is updated
enum class IntrinsicID : u8 {
	matrix_multiply = 0,
	matrix_transpose,
	matrix_column_major_load,
	matrix_column_major_store,
	read_cycle_counter,
	read_steady_counter,
	give_address,
	caller_give_address,
	thread_pointer,
};

class MetaIntrinsic final : public Expression {
	PrerunExpression*      name;
	Vec<PrerunExpression*> genArgs;

	Vec<Expression*> arguments;

  public:
	MetaIntrinsic(PrerunExpression* _name, Vec<PrerunExpression*> _genArgs, Vec<Expression*> _arguments,
	              FileRangePtr _fileRange)
	    : Expression(_fileRange), name(_name), genArgs(std::move(_genArgs)), arguments(std::move(_arguments)) {}

	static MetaIntrinsic* create(PrerunExpression* name, Vec<PrerunExpression*> genArgs, Vec<Expression*> arguments,
	                             FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(MetaIntrinsic), name, std::move(genArgs), std::move(arguments), _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(name);
		for (auto arg : genArgs) {
			UPDATE_DEPS(arg);
		}
		for (auto arg : arguments) {
			UPDATE_DEPS(arg);
		}
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::META_INTRINSIC; }
};

} // namespace qat::ast

#endif
