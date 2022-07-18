#ifndef QAT_AST_EXPRESSIONS_SIZE_OF_HPP
#define QAT_AST_EXPRESSIONS_SIZE_OF_HPP

#include "../expression.hpp"

namespace qat::ast {

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
   * @param _fileRange
   */
  SizeOf(Expression *_expression, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::sizeOf; }
};

} // namespace qat::ast

#endif