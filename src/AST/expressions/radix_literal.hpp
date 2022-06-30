#ifndef QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP

#include "../../IR/generator.hpp"
#include "../expression.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

class RadixLiteral : public Expression {
private:
  // String representation of the integer
  std::string value;

  // Radix value
  unsigned radix;

public:
  /**
   *  A radix integer Literal
   *
   * @param _value String representation of the integer
   * @param _radix Radix value
   * @param _filePlacement
   */
  RadixLiteral(std::string _value, unsigned _radix,
               utils::FilePlacement _filePlacement);

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
  NodeType nodeType() { return qat::AST::NodeType::radixLiteral; }
};

} // namespace AST
} // namespace qat

#endif