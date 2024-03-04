#ifndef QAT_AST_PRERUN_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_PRERUN_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

class CustomFloatLiteral final : public PrerunExpression, public TypeInferrable {
  String value;
  String kind;

public:
  CustomFloatLiteral(String _value, String _kind, FileRange _fileRange)
      : PrerunExpression(_fileRange), value(_value), kind(_kind) {}

  useit static inline CustomFloatLiteral* create(String _value, String _kind, FileRange _fileRange) {
    return std::construct_at(OwnNormal(CustomFloatLiteral), _value, _kind, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {}

  useit ir::PrerunValue* emit(EmitCtx* ctx) override;
  useit Json             to_json() const override;
  useit String           to_string() const override;
  useit NodeType         nodeType() const override { return NodeType::CUSTOM_FLOAT_LITERAL; }
};

} // namespace qat::ast

#endif