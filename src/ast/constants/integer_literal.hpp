#ifndef QAT_AST_CONSTANTS_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral : public ConstantExpression {
private:
  String               value;
  mutable IR::QatType* expected = nullptr;

public:
  IntegerLiteral(String _value, FileRange _fileRange);

  void  setType(IR::QatType* ty) const;
  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit NodeType           nodeType() const override { return NodeType::integerLiteral; }
};

} // namespace qat::ast

#endif