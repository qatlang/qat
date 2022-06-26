#ifndef QAT_AST_EXPRESSIONS_NULL_POINTER_HPP
#define QAT_AST_EXPRESSIONS_NULL_POINTER_HPP

#include "../expression.hpp"
#include "llvm/IR/Type.h"

namespace qat {
namespace AST {

/**
 *  A null pointer
 *
 */
class NullPointer : public Expression {
private:
  /**
   *  Type of the pointer
   *
   */
  llvm::Type *type;

public:
  /**
   *  A null pointer
   *
   * @param _filePlacement
   */
  NullPointer(utils::FilePlacement _filePlacement);

  /**
   *  A null pointer of the specified type
   *
   * @param _type LLVM Type of the pointer
   * @param _filePlacement
   */
  NullPointer(llvm::Type *_type, utils::FilePlacement _filePlacement);

  /**
   *  Set the type for this null pointer
   *
   * @param type Type of the pointer
   */
  void set_type(llvm::Type *type);

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
  NodeType nodeType() { return NodeType::nullPointer; }
};

} // namespace AST
} // namespace qat

#endif