#ifndef QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

/**
 *  FunctionCall represents a normal call to a function in the language
 *
 */
class FunctionCall : public Expression {
private:
  /**
   *  Name of the function to call
   *
   */
  std::string name;

  /**
   *  Expressions for the arguments to be passed
   *
   */
  std::vector<Expression *> arguments;

public:
  /**
   *  FunctionCall represents a normal/static function call in the
   * language
   *
   * @param _name Name of the function to call
   * @param _arguments Expressions representing the value of the arguments to be
   * passed
   * @param _fileRange
   */
  FunctionCall(std::string _name, std::vector<Expression *> _arguments,
               utils::FileRange _fileRange)
      : name(_name), arguments(_arguments), Expression(_fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::functionCall; }
};

} // namespace qat::ast

#endif