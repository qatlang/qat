#ifndef QAT_AST_CONSTANTS_STRING_LITERAL_HPP
#define QAT_AST_CONSTANTS_STRING_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

// StringLiteral is used to represent literal strings
class StringLiteral : public ConstantExpression {
private:
  // Value of the string
  String value;

public:
  // StringLiteral is used to represent literal strings
  StringLiteral(String _value, utils::FileRange _fileRange);

  void addValue(const String& val, const utils::FileRange& fRange);

  // Get the value of the string
  useit String get_value() const;
  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit NodeType           nodeType() const override { return NodeType::stringLiteral; }
};

} // namespace qat::ast

#endif