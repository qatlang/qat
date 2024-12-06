#ifndef QAT_AST_PRERUN_BINARY_EXPRESSION_HPP
#define QAT_AST_PRERUN_BINARY_EXPRESSION_HPP

#include "../expression.hpp"
#include "../expressions/operator.hpp"

namespace qat::ast {

class PrerunBinaryOp final : public PrerunExpression {
  PrerunExpression* lhs;
  Op                opr;
  PrerunExpression* rhs;

public:
  PrerunBinaryOp(PrerunExpression* _lhs, Op _opr, PrerunExpression* _rhs, FileRange _fileRange)
      : PrerunExpression(_fileRange), lhs(_lhs), opr(_opr), rhs(_rhs) {}

  useit static PrerunBinaryOp* create(PrerunExpression* _lhs, Op _opr, PrerunExpression* _rhs, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunBinaryOp), _lhs, _opr, _rhs, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(lhs);
    UPDATE_DEPS(rhs);
  }

  useit ir::PrerunValue* emit(EmitCtx* ctx);
  useit String           to_string() const final;
  useit Json             to_json() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_BINARY_OP; }
};

} // namespace qat::ast

#endif
