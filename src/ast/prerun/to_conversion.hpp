#ifndef QAT_AST_PRERUN_TO_CONVERSION_HPP
#define QAT_AST_PRERUN_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunTo final : public PrerunExpression {
  PrerunExpression* value;
  Type*             targetType;

public:
  PrerunTo(PrerunExpression* _value, Type* _targetType, FileRange _fileRange)
      : PrerunExpression(_fileRange), value(_value), targetType(_targetType) {}

  useit static PrerunTo* create(PrerunExpression* _value, Type* _targetType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunTo), _value, _targetType, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

  useit ir::PrerunValue* emit(EmitCtx* ctx);
  useit String           to_string() const final;
  useit Json             to_json() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_TO_CONVERSION; }
};

} // namespace qat::ast

#endif
