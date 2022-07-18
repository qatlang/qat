#ifndef QAT_AST_EXPRESSION_HPP
#define QAT_AST_EXPRESSION_HPP

#include "./node.hpp"

namespace qat {
namespace AST {

// Kind of the expression
//
// "assignable" & "temporary" can be grouped to the generalised kind glvalue
// "pure" and "temporary" can be grouped to the generalsied kind rvalue
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
  Expression(utils::FilePlacement _filePlacement);

  virtual ~Expression() {}

  void setExpectedKind(ExpressionKind _kind);

  ExpressionKind getExpectedKind();

  bool isExpectedKind(ExpressionKind _kind);

  virtual IR::Value *emit(IR::Context *ctx){};

  virtual void emitCPP(backend::cpp::File &file, bool isHeader) const {};

  virtual NodeType nodeType() const {};

  virtual nuo::Json toJson() const {};
};
} // namespace AST
} // namespace qat

#endif