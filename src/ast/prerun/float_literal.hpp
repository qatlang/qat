#ifndef QAT_AST_CONSTANTS_FLOAT_LITERAL_HPP
#define QAT_AST_CONSTANTS_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FloatLiteral : public PrerunExpression, public TypeInferrable {
private:
  String value;

public:
  FloatLiteral(String _value, FileRange _fileRange) : PrerunExpression(_fileRange), value(_value) {}

  useit static inline FloatLiteral* create(String _value, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FloatLiteral), _value, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const override;
  useit NodeType         nodeType() const override { return NodeType::FLOAT_LITERAL; }
};

} // namespace qat::ast

#endif