#ifndef QAT_AST_CONSTANTS_BOOLEAN_LITERAL_HPP
#define QAT_AST_CONSTANTS_BOOLEAN_LITERAL_HPP

#include "../expression.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

class BooleanLiteral : public ConstantExpression {
private:
  bool value;

public:
  BooleanLiteral(bool _value, FileRange _fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) final;
  useit Json               toJson() const final;
  useit NodeType           nodeType() const final { return NodeType::booleanLiteral; }
};

} // namespace qat::ast

#endif