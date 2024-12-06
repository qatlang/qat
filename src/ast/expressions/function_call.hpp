#ifndef QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FunctionCall final : public Expression {
private:
  Expression*      fnExpr;
  Vec<Expression*> values;

public:
  FunctionCall(Expression* _fnExpr, Vec<Expression*> _arguments, FileRange _fileRange)
      : Expression(std::move(_fileRange)), fnExpr(_fnExpr), values(_arguments) {}

  useit static FunctionCall* create(Expression* _fnExpr, Vec<Expression*> _arguments, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FunctionCall), _fnExpr, _arguments, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(fnExpr);
    for (auto arg : values) {
      UPDATE_DEPS(arg);
    }
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit Json       to_json() const final;
  useit NodeType   nodeType() const final { return NodeType::FUNCTION_CALL; }
};

} // namespace qat::ast

#endif
