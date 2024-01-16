#ifndef QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FunctionCall : public Expression {
private:
  Expression*      fnExpr;
  Vec<Expression*> values;

public:
  FunctionCall(Expression* _fnExpr, Vec<Expression*> _arguments, FileRange _fileRange)
      : Expression(std::move(_fileRange)), fnExpr(_fnExpr), values(_arguments) {}

  useit static inline FunctionCall* create(Expression* _fnExpr, Vec<Expression*> _arguments, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FunctionCall), _fnExpr, _arguments, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::FUNCTION_CALL; }
};

} // namespace qat::ast

#endif