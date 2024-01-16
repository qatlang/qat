#ifndef QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP
#define QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP

#include "../expression.hpp"

#include <vector>

namespace qat::ast {

class TupleValue : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class LocalDeclaration;

  bool             isPacked = false;
  Vec<Expression*> members;

public:
  TupleValue(Vec<Expression*> _members, FileRange _fileRange) : Expression(_fileRange), members(_members) {}

  useit static inline TupleValue* create(Vec<Expression*> _members, FileRange _fileRange) {
    return std::construct_at(OwnNormal(TupleValue), _members, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::TUPLE_VALUE; }
};

} // namespace qat::ast

#endif