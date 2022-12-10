#ifndef QAT_AST_CONSTANTS_NULL_POINTER_HPP
#define QAT_AST_CONSTANTS_NULL_POINTER_HPP

#include "../expression.hpp"

namespace qat::ast {

/**
 *  A null pointer
 *
 */
class NullPointer : public ConstantExpression {
private:
  // Type of the pointer
  IR::PointerType* candidateType = nullptr;

public:
  explicit NullPointer(FileRange _fileRange);

  void setType(IR::PointerType* typ);

  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit NodeType           nodeType() const override { return NodeType::nullPointer; }
};

} // namespace qat::ast

#endif