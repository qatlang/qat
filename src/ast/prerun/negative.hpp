#ifndef QAT_AST_PRERUN_NEGATIVE_HPP
#define QAT_AST_PRERUN_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunNegative final : public PrerunExpression, public TypeInferrable {
  PrerunExpression* value;

public:
  PrerunNegative(PrerunExpression* _value, FileRange _fileRange) : PrerunExpression(_fileRange), value(_value) {}

  useit static inline PrerunNegative* create(PrerunExpression* value, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunNegative), value, fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(value);
  }

  IR::PrerunValue* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit String     toString() const;
  useit NodeType   nodeType() const { return NodeType::PRERUN_NEGATIVE; }
};

} // namespace qat::ast

#endif