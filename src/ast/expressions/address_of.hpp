#ifndef QAT_AST_ADDRESS_OF_HPP
#define QAT_AST_ADDRESS_OF_HPP

#include "../expression.hpp"

namespace qat::ast {

class AddressOf final : public Expression {
	Expression* instance;

  public:
	AddressOf(Expression* _instance, FileRangePtr _fileRange) : Expression(_fileRange), instance(_instance) {}

	static AddressOf* create(Expression* _instance, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(AddressOf), _instance, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(instance);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::ADDRESS_OF; }
};

} // namespace qat::ast

#endif
