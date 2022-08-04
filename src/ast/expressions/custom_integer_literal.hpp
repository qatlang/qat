#ifndef QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/helpers.hpp"
#include "../expression.hpp"

namespace qat::ast {

class CustomIntegerLiteral : public Expression {
private:
  String value;

  u64 bitWidth;

  bool isUnsigned;

public:
  CustomIntegerLiteral(String _value, bool _isUnsigned, u32 _bitWidth,
                       utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::customIntegerLiteral;
  }
};

} // namespace qat::ast

#endif