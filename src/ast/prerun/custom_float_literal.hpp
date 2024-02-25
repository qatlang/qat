#ifndef QAT_AST_CONSTANTS_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_FLOAT_LITERAL_HPP

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

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {}

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const override;
  useit NodeType         nodeType() const override { return NodeType::CUSTOM_FLOAT_LITERAL; }
};

} // namespace qat::ast

#endif