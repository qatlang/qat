#ifndef QAT_AST_CONSTANTS_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral : public PrerunExpression, public TypeInferrable {
private:
  String value;

public:
  IntegerLiteral(String _value, FileRange _fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const override;
  useit NodeType         nodeType() const override { return NodeType::integerLiteral; }
};

} // namespace qat::ast

#endif