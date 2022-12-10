#ifndef QAT_AST_EXPRESSION_HPP
#define QAT_AST_EXPRESSION_HPP

#include "./node.hpp"

namespace qat::ast {

// Nature of the expression
//
// "assignable" & "temporary" can be grouped to the generalised nature glvalue
// "pure" and "temporary" can be grouped to the generalsied nature rvalue
//
// Assignable expressions can be assigned to
enum class ExpressionKind {
  assignable, // lvalue
  temporary,  // xvalue
  pure,       // prvalue
};

class Expression : public Node {
private:
  ExpressionKind expected;

public:
  Expression(FileRange _fileRange);

  void                 setExpectedKind(ExpressionKind _kind);
  useit ExpressionKind getExpectedKind();
  useit bool           isExpectedKind(ExpressionKind _kind);
  useit IR::Value* emit(IR::Context* ctx) override = 0;
  useit NodeType   nodeType() const override       = 0;
  useit Json       toJson() const override         = 0;
  ~Expression() override                           = default;
};

class ConstantExpression : public Expression {
public:
  ConstantExpression(FileRange fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) override = 0;
  useit NodeType           nodeType() const override       = 0;
  useit Json               toJson() const override         = 0;
  ~ConstantExpression() override                           = default;
};

} // namespace qat::ast

#endif