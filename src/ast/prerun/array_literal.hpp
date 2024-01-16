#ifndef QAT_AST_ARRAY_LITERAL_HPP
#define QAT_AST_ARRAY_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunArrayLiteral : public PrerunExpression, public TypeInferrable {
  Vec<PrerunExpression*> valuesExp;

public:
  PrerunArrayLiteral(Vec<PrerunExpression*> _elements, FileRange _fileRange)
      : PrerunExpression(_fileRange), valuesExp(_elements) {}

  useit static inline PrerunArrayLiteral* create(Vec<PrerunExpression*> elements, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunArrayLiteral), elements, fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_ARRAY_LITERAL; }
};

} // namespace qat::ast

#endif