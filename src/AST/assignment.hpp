#ifndef QAT_AST_ASSIGNMENT_HPP
#define QAT_AST_ASSIGNMENT_HPP

#include "../IR/generator.hpp"
#include "../utils/cast_if_null_pointer.hpp"
#include "../utils/llvm_type_to_name.hpp"
#include "../utils/variability.hpp"
#include "./expression.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <string>

namespace qat {
namespace AST {

/**
 * @brief Assignment represents storage of a value to a variable in the
 * language. This will throw errors if the entity to be reassigned is not a
 * variable.
 */
class Assignment : public Sentence {
private:
  /**
   * @brief Name of the variable to which an expression has to be assigned
   *
   */
  std::string name;

  /**
   * @brief Expression instance that represents the expression that has to be
   * assigned to the variable
   *
   */
  Expression value;

public:
  /**
   * @brief Assignment represents storage of a value to a variable in the
   * language. This will throw errors if the entity to be reassigned is not a
   * variable
   *
   * @param _name Name of the variable that has to be reassigned
   * @param _value The Expression instance that represents the expression that
   * should be assigned to the variable
   * @param _filePlacement FilePLacement instance that represents the range
   * spanned by the tokens making up this AST member
   */
  Assignment(std::string _name, Expression _value,
             utils::FilePlacement _filePlacement);

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   * @brief Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return qat::AST::NodeType::reassignment; }
};

} // namespace AST
} // namespace qat

#endif