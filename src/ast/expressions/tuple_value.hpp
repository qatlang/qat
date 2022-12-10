#ifndef QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP
#define QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP

#include "../expression.hpp"

#include <vector>

namespace qat::ast {

class TupleValue : public Expression {
  Vec<Expression*> members;

public:
  TupleValue(Vec<Expression*> _members, FileRange _fileRange);

  IR::Value* emit(IR::Context* ctx);

  Json toJson() const;

  NodeType nodeType() const { return NodeType::tupleValue; }
};

} // namespace qat::ast

#endif