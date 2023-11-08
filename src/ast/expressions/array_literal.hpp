#ifndef QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP

#include "../expression.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

class ArrayLiteral : public Expression, public LocalDeclCompatible, public InPlaceCreatable, public TypeInferrable {
  friend class LocalDeclaration;
  friend class Assignment;

private:
  Vec<Expression*> values;

public:
  ArrayLiteral(Vec<Expression*> _values, FileRange _fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::arrayLiteral; }
};

} // namespace qat::ast

#endif