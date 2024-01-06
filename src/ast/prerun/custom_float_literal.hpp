#ifndef QAT_AST_CONSTANTS_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

class CustomFloatLiteral : public PrerunExpression, public TypeInferrable {
private:
  // Numerical value of the float
  String value;

  // Nature of the float
  String kind;

public:
  CustomFloatLiteral(String _value, String _kind, FileRange _fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const override;
  useit NodeType         nodeType() const override { return NodeType::customFloatLiteral; }
};

} // namespace qat::ast

#endif