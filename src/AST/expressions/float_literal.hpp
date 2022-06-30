#ifndef QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP

#include "../../IR/generator.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

class FloatLiteral : public Expression {
private:
  /**
   *  String representation of the floating point number
   *
   */
  std::string value;

public:
  /**
   *  A Float Literal
   *
   * @param _value String representation of the floating point number
   * @param _filePlacement
   */
  FloatLiteral(std::string _value, utils::FilePlacement _filePlacement);

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  backend::JSON toJSON() const;

  NodeType nodeType() { return qat::AST::NodeType::floatLiteral; }
};

} // namespace AST
} // namespace qat

#endif