#ifndef QAT_AST_CAST_HPP
#define QAT_AST_CAST_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Cast final : public Expression {
	Expression* instance;
	Type*       destination;

  public:
	Cast(Expression* _mainExp, Type* _dest, FileRangePtr _fileRange)
	    : Expression(_fileRange), instance(_mainExp), destination(_dest) {}

	static Cast* create(Expression* mainExp, Type* value, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Cast), mainExp, value, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(destination);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::CAST; }
};

} // namespace qat::ast

#endif
