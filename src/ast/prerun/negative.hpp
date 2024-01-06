#ifndef QAT_AST_PRERUN_NEGATIVE_HPP
#define QAT_AST_PRERUN_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunNegative : public PrerunExpression, public TypeInferrable {
  PrerunExpression* value;

public:
  PrerunNegative(PrerunExpression* value, FileRange fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  IR::PrerunValue* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::prerunNegative; }
};

} // namespace qat::ast

#endif