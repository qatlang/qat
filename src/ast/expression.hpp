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
  Expression(utils::FileRange _fileRange);

  ~Expression() override = default;

  void setExpectedKind(ExpressionKind _kind);

  ExpressionKind getExpectedKind();

  bool isExpectedKind(ExpressionKind _kind);

  IR::Value *emit(IR::Context *ctx) override = 0;

  NodeType nodeType() const override = 0;

  nuo::Json toJson() const override = 0;
};

} // namespace qat::ast

#endif