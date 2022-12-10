#ifndef QAT_AST_CONSTANTS_SIZE_OF_TYPE_HPP
#define QAT_AST_CONSTANTS_SIZE_OF_TYPE_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

// SizeOfType is used to find the size of a type specified
class SizeOfType : public ConstantExpression {
private:
  // Type to find the size of
  QatType* type;

public:
  // SizeOfType is used to find the size of a type specified
  SizeOfType(QatType* _type, FileRange _fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit NodeType           nodeType() const override { return NodeType::sizeOfType; }
};

} // namespace qat::ast

#endif