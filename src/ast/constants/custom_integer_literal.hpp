#ifndef QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/helpers.hpp"
#include "../expression.hpp"

namespace qat::ast {

class CustomIntegerLiteral : public ConstantExpression {
private:
  String value;

  u64 bitWidth;

  bool isUnsigned;

public:
  CustomIntegerLiteral(String _value, bool _isUnsigned, u32 _bitWidth, FileRange _fileRange);

  IR::ConstantValue* emit(IR::Context* ctx) override;

  useit Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::customIntegerLiteral; }
};

} // namespace qat::ast

#endif