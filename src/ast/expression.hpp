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

protected:
  Maybe<IR::QatType*> inferredType;

public:
  Expression(FileRange _fileRange);

  bool isTypeInferred() const;
  void setInferenceType(IR::QatType* type);

  void                 setExpectedKind(ExpressionKind _kind);
  useit ExpressionKind getExpectedKind();
  useit bool           isExpectedKind(ExpressionKind _kind);
  useit IR::Value* emit(IR::Context* ctx) override = 0;
  useit NodeType   nodeType() const override       = 0;
  useit Json       toJson() const override         = 0;
  ~Expression() override                           = default;
};

class PrerunExpression : public Expression {
public:
  PrerunExpression(FileRange fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx) override = 0;
  useit NodeType         nodeType() const override       = 0;
  useit Json             toJson() const override         = 0;
  useit virtual String   toString() const;
  ~PrerunExpression() override = default;
};

} // namespace qat::ast

#endif