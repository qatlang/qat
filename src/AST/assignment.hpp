#ifndef QAT_AST_ASSIGNMENT_HPP
#define QAT_AST_ASSIGNMENT_HPP

#include "../IR/generator.hpp"
#include "../utils/cast_if_null_pointer.hpp"
#include "../utils/llvm_type_to_name.hpp"
#include "../utils/pointer_kind.hpp"
#include "../utils/variability.hpp"
#include "./expression.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include <string>

namespace qat {
namespace AST {

/**
 *  Assignment represents storage of a value to a variable in the
 * language. This will throw errors if the entity to be reassigned is not a
 * variable.
 */
class Assignment : public Sentence {
private:
  /**
   *  The left hand side of the expression
   *
   */
  Expression *lhs;

  /**
   *  Expression instance that represents the expression that has to be
   * assigned to the variable
   *
   */
  Expression *value;

public:
  /**
   *  Assignment represents storage of a value to a variable in the
   * language. This will throw errors if the entity to be reassigned is not a
   * variable
   *
   * @param _lhs Expression to be reassigned to
   * @param _value The Expression instance that represents the expression that
   * should be assigned to the variable
   * @param _filePlacement FilePLacement instance that represents the range
   * spanned by the tokens making up this AST member
   */
  Assignment(Expression *_lhs, Expression *_value,
             utils::FilePlacement _filePlacement);

  llvm::Value *generate(IR::Generator *generator);

  backend::JSON toJSON() const;

  NodeType nodeType() { return qat::AST::NodeType::reassignment; }
};

} // namespace AST
} // namespace qat

#endif