#ifndef QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP

#include "../../IR/generator.hpp"
#include "../expression.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

class IntegerLiteral : public Expression {
private:
  /**
   *  String representation of the integer
   *
   */
  std::string value;

public:
  /**
   *  An Integer Literal
   *
   * @param _value String representation of the integer
   * @param _filePlacement
   */
  IntegerLiteral(std::string _value, utils::FilePlacement _filePlacement);

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   *  Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return qat::AST::NodeType::integerLiteral; }
};

} // namespace AST
} // namespace qat

#endif