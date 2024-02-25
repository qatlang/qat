#ifndef QAT_AST_ARRAY_LITERAL_HPP
#define QAT_AST_ARRAY_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunArrayLiteral final : public PrerunExpression, public TypeInferrable {
  Vec<PrerunExpression*> valuesExp;

public:
  PrerunArrayLiteral(Vec<PrerunExpression*> _elements, FileRange _fileRange)
      : PrerunExpression(_fileRange), valuesExp(_elements) {}

  useit static inline PrerunArrayLiteral* create(Vec<PrerunExpression*> elements, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunArrayLiteral), elements, fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    for (auto val : valuesExp) {
      UPDATE_DEPS(val);
    }
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit String           toString() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_ARRAY_LITERAL; }
};

} // namespace qat::ast

#endif