#ifndef QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP
#define QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ToConversion final : public Expression {
	Expression* source;
	Type*       destinationType;

  public:
	ToConversion(Expression* _source, Type* _destinationType, FileRangePtr _fileRange)
	    : Expression(_fileRange), source(_source), destinationType(_destinationType) {}

	static ToConversion* create(Expression* _source, Type* _destinationType, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ToConversion), _source, _destinationType, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(source);
		UPDATE_DEPS(destinationType);
	}

	ir::Value* emit(EmitCtx* ctx) override;

	NodeType nodeType() const override { return NodeType::TO_CONVERSION; };
};

} // namespace qat::ast

#endif
