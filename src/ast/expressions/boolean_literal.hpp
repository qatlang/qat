#ifndef QAT_AST_EXPRESSIONS_BOOLEAN_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_BOOLEAN_LITERAL_HPP

#include "../expression.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

class BooleanLiteral : public Expression {
private:
  bool value;

public:
  BooleanLiteral(bool _value, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::booleanLiteral; }
};

} // namespace qat::ast

#endif