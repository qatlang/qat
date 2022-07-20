#ifndef QAT_AST_SENTENCES_ASSIGNMENT_HPP
#define QAT_AST_SENTENCES_ASSIGNMENT_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"

#include <string>

namespace qat::ast {

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
   * @param _fileRange FilePLacement instance that represents the range
   * spanned by the tokens making up this AST member
   */
  Assignment(Expression *_lhs, Expression *_value, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::reassignment; }
};

} // namespace qat::ast

#endif