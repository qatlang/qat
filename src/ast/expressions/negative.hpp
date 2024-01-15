#ifndef QAT_AST_NEGATIVE_HPP
#define QAT_AST_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Negative : public Expression, public TypeInferrable {
  Expression* value;

public:
  Negative(Expression* value, FileRange fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::negative; }
};

} // namespace qat::ast

#endif