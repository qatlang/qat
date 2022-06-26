#ifndef QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/generator.hpp"
#include "../../utils/types.hpp"
#include "../expression.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

class CustomIntegerLiteral : public Expression {
private:
  std::string value;
  u64 bitWidth;
  bool isUnsigned;

public:
  CustomIntegerLiteral(std::string _value, bool _isUnsigned,
                       unsigned int _bitWidth,
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
  NodeType nodeType() { return qat::AST::NodeType::customIntegerLiteral; }
};
} // namespace AST
} // namespace qat

#endif