#ifndef QAT_AST_CONSTANTS_NULL_POINTER_HPP
#define QAT_AST_CONSTANTS_NULL_POINTER_HPP

#include "../expression.hpp"

namespace qat::ast {

/**
 *  A null pointer
 *
 */
class NullPointer : public ConstantExpression {
public:
  explicit NullPointer(FileRange _fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit String             toString() const final;
  useit NodeType           nodeType() const override { return NodeType::nullPointer; }
};

} // namespace qat::ast

#endif