#ifndef QAT_AST_EXPRESSIONS_STRING_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_STRING_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

// StringLiteral is used to represent literal strings
class StringLiteral : public Expression {
private:
  // Value of the string
  String value;

public:
  // StringLiteral is used to represent literal strings
  StringLiteral(String _value, utils::FileRange _fileRange);

  // Get the value of the string
  useit String get_value() const;

  IR::Value *emit(IR::Context *ctx) override;

  useit NodeType nodeType() const override { return NodeType::stringLiteral; }

  useit nuo::Json toJson() const override;
};

} // namespace qat::ast

#endif