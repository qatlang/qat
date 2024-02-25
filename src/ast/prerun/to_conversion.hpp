#ifndef QAT_AST_PRERUN_TO_CONVERSION_HPP
#define QAT_AST_PRERUN_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunTo final : public PrerunExpression {
  PrerunExpression* value;
  QatType*          targetType;

public:
  PrerunTo(PrerunExpression* _value, QatType* _targetType, FileRange _fileRange)
      : PrerunExpression(_fileRange), value(_value), targetType(_targetType) {}

  useit static inline PrerunTo* create(PrerunExpression* _value, QatType* _targetType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunTo), _value, _targetType, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final;

  useit IR::PrerunValue* emit(IR::Context* ctx);
  useit String           toString() const final;
  useit Json             toJson() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_TO_CONVERSION; }
};

} // namespace qat::ast

#endif