#ifndef QAT_AST_ARRAY_LITERAL_HPP
#define QAT_AST_ARRAY_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunArrayLiteral : public PrerunExpression, public TypeInferrable {
  Vec<PrerunExpression*> valuesExp;

public:
  useit static PrerunArrayLiteral* Create(Vec<PrerunExpression*> elements, FileRange fileRange);

  PrerunArrayLiteral(Vec<PrerunExpression*> _elements, FileRange _fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit NodeType         nodeType() const final { return NodeType::prerunArrayLiteral; }
};

} // namespace qat::ast

#endif