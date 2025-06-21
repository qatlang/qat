#ifndef QAT_AST_EXPRESSIONS_THIS_HPP
#define QAT_AST_EXPRESSIONS_THIS_HPP

#include "../expression.hpp"

namespace qat::ast {

class SelfInstance final : public Expression {
  public:
	explicit SelfInstance(FileRange _fileRange) : Expression(_fileRange) {}

	useit static SelfInstance* create(FileRange _fileRange) {
		return std::construct_at(OwnNormal(SelfInstance), _fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	useit ir::Value* emit(EmitCtx* ctx) override;
	useit Json       to_json() const override;

	useit NodeType nodeType() const override { return NodeType::SELF; }
};

} // namespace qat::ast

#endif
