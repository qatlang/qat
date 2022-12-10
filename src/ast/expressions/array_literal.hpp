#ifndef QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP

#include "../expression.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

class ArrayLiteral : public Expression {
  friend class LocalDeclaration;

private:
  Vec<Expression*> values;

  //

  IR::LocalValue* local;
  String          name;
  bool            isVar;

public:
  ArrayLiteral(Vec<Expression*> _values, FileRange _fileRange);

  useit bool hasLocal() const;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::arrayLiteral; }
};

} // namespace qat::ast

#endif