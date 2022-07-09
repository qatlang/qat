#ifndef QAT_AST_EXPRESSIONS_SIZE_OF_HPP
#define QAT_AST_EXPRESSIONS_SIZE_OF_HPP

#include "../expression.hpp"
#include "llvm/IR/Constants.h"

namespace qat {
namespace AST {

/**
 *  SizeOf is used to find the size of the type of the provided Expression
 *
 */
class SizeOf : public Expression {
private:
  /**
   *  Expression for which the size of the type has to be calculated
   *
   */
  Expression *expression;

public:
  /**
   *  SizeOf is used to find the size of the type of the provided
   * Expression
   *
   * @param _expression Expression to be used to find the size of the type
   * @param _filePlacement
   */
  SizeOf(Expression *_expression, utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::sizeOf; }
};

} // namespace AST
} // namespace qat

#endif