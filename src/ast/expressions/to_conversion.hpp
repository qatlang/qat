#ifndef QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP
#define QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ToConversion final : public Expression {
  Expression* source;
  QatType*    destinationType;

public:
  ToConversion(Expression* _source, QatType* _destinationType, FileRange _fileRange)
      : Expression(_fileRange), source(_source), destinationType(_destinationType) {}

  useit static inline ToConversion* create(Expression* _source, QatType* _destinationType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ToConversion), _source, _destinationType, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(source);
    UPDATE_DEPS(destinationType);
  }

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::TO_CONVERSION; };
};

} // namespace qat::ast

#endif