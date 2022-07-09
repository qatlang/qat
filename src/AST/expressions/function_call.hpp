#ifndef QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat {
namespace AST {

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
   * @param _filePlacement
   */
  FunctionCall(std::string _name, std::vector<Expression *> _arguments,
               utils::FilePlacement _filePlacement)
      : name(_name), arguments(_arguments), Expression(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return qat::AST::NodeType::functionCall; }
};
} // namespace AST
} // namespace qat

#endif