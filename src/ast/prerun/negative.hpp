#ifndef QAT_AST_PRERUN_NEGATIVE_HPP
#define QAT_AST_PRERUN_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunNegative : public PrerunExpression, public TypeInferrable {
  PrerunExpression* value;

public:
  PrerunNegative(PrerunExpression* _value, FileRange _fileRange) : PrerunExpression(_fileRange), value(_value) {}

  useit static inline PrerunNegative* create(PrerunExpression* value, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunNegative), value, fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  IR::PrerunValue* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::PRERUN_NEGATIVE; }
};

} // namespace qat::ast

#endif