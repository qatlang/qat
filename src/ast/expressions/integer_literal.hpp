#ifndef QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral : public Expression {
private:
  String value;

public:
  IntegerLiteral(String _value, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::integerLiteral; }
};

} // namespace qat::ast

#endif