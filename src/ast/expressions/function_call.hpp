#ifndef QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

// FunctionCall represents a normal call to a function in the language
class FunctionCall : public Expression {
private:
  // Name of the function to call
  String name;
  // Expressions for the arguments to be passed
  Vec<Expression *> arguments;

public:
  /**
   * FunctionCall represents a normal/static function call in the
   * language
   *
   * @param _name Name of the function to call
   * @param _arguments Expressions representing the value of the arguments to be
   * passed
   * @param _fileRange
   */
  FunctionCall(String _name, Vec<Expression *> _arguments,
               utils::FileRange _fileRange)
      : Expression(std::move(_fileRange)), name(std::move(_name)),
        arguments(std::move(_arguments)) {}

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::functionCall; }
};

} // namespace qat::ast

#endif