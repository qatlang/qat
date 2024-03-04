#ifndef QAT_AST_PRERUN_UNARY_OPERATOR_HPP
#define QAT_AST_PRERUN_UNARY_OPERATOR_HPP

#include "../expression.hpp"
#include "../expressions/operator.hpp"

namespace qat::ast {

class PrerunBitwiseNot final : public PrerunExpression {
  PrerunExpression* value;

public:
  PrerunBitwiseNot(PrerunExpression* _value, FileRange _fileRange) : PrerunExpression(_fileRange), value(_value) {}

  useit static inline PrerunBitwiseNot* create(PrerunExpression* _value, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunBitwiseNot), _value, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(value);
  }

  useit ir::PrerunValue* emit(EmitCtx* ctx);
  useit String           to_string() const final;
  useit Json             to_json() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_BITWISE_NOT; }
};

} // namespace qat::ast

#endif