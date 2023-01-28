#ifndef QAT_AST_CONSTANTS_FLOAT_LITERAL_HPP
#define QAT_AST_CONSTANTS_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FloatLiteral : public ConstantExpression {
private:
  String value;

public:
  FloatLiteral(String _value, FileRange _fileRange);

  IR::ConstantValue* emit(IR::Context* ctx) override;

  useit Json toJson() const override;

  useit String toString() const override;

  useit NodeType nodeType() const override { return NodeType::floatLiteral; }
};

} // namespace qat::ast

#endif