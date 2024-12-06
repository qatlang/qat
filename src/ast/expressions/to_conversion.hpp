#ifndef QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP
#define QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ToConversion final : public Expression {
  Expression* source;
  Type*       destinationType;

public:
  ToConversion(Expression* _source, Type* _destinationType, FileRange _fileRange)
      : Expression(_fileRange), source(_source), destinationType(_destinationType) {}

  useit static ToConversion* create(Expression* _source, Type* _destinationType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ToConversion), _source, _destinationType, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(source);
    UPDATE_DEPS(destinationType);
  }

  useit ir::Value* emit(EmitCtx* ctx) override;
  useit Json       to_json() const override;
  useit NodeType   nodeType() const override { return NodeType::TO_CONVERSION; };
};

} // namespace qat::ast

#endif
