#ifndef QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

// FunctionCall represents a normal call to a function in the language
class FunctionCall : public Expression {
private:
  Expression       *fnExpr;
  Vec<Expression *> arguments;

public:
  FunctionCall(Expression *_fnExpr, Vec<Expression *> _arguments,
               utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::functionCall; }
};

} // namespace qat::ast

#endif