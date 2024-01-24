#ifndef QAT_AST_PRERUN_TUPLE_VALUE_HPP
#define QAT_AST_PRERUN_TUPLE_VALUE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunTupleValue : public PrerunExpression, public TypeInferrable {
  Vec<PrerunExpression*> members;

public:
  PrerunTupleValue(Vec<PrerunExpression*> _members, FileRange _fileRange)
      : PrerunExpression(_fileRange), members(_members) {}

  useit static inline PrerunTupleValue* create(Vec<PrerunExpression*> _members, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunTupleValue), _members, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit String           toString() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_TUPLE_VALUE; }
};

} // namespace qat::ast

#endif