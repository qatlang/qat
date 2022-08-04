#ifndef QAT_AST_EXPRESSIONS_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

class CustomFloatLiteral : public Expression {
private:
  // Numerical value of the float
  String value;

  // Nature of the float
  String kind;

public:
  CustomFloatLiteral(String _value, String _kind, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::customFloatLiteral;
  }
};

} // namespace qat::ast

#endif