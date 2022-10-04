#ifndef QAT_AST_CONSTANTS_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

class CustomFloatLiteral : public ConstantExpression {
private:
  // Numerical value of the float
  String value;

  // Nature of the float
  String kind;

public:
  CustomFloatLiteral(String _value, String _kind, utils::FileRange _fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit NodeType           nodeType() const override { return NodeType::customFloatLiteral; }
};

} // namespace qat::ast

#endif