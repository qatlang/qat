#ifndef QAT_AST_CONSTANTS_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral final : public PrerunExpression, public TypeInferrable {
private:
  String                      value;
  Maybe<Pair<u64, FileRange>> bits;

public:
  IntegerLiteral(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange)
      : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bits(_bits) {}

  useit static inline IntegerLiteral* create(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange) {
    return std::construct_at(OwnNormal(IntegerLiteral), _value, _bits, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase, Maybe<IR::DependType>, IR::EntityState*, IR::Context*) final {}

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const override;
  useit NodeType         nodeType() const override { return NodeType::INTEGER_LITERAL; }
};

} // namespace qat::ast

#endif