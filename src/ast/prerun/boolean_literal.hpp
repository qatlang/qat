#ifndef QAT_AST_CONSTANTS_BOOLEAN_LITERAL_HPP
#define QAT_AST_CONSTANTS_BOOLEAN_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class BooleanLiteral final : public PrerunExpression {
  bool value;

public:
  BooleanLiteral(bool _value, FileRange _fileRange) : PrerunExpression(std::move(_fileRange)), value(_value) {}

  useit static inline BooleanLiteral* create(bool _value, FileRange _fileRange) {
    return std::construct_at(OwnNormal(BooleanLiteral), _value, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {}

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit String           toString() const final;
  useit NodeType         nodeType() const final { return NodeType::BOOLEAN_LITERAL; }
};

} // namespace qat::ast

#endif