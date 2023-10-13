#ifndef QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/helpers.hpp"
#include "../expression.hpp"

namespace qat::ast {

class CustomIntegerLiteral : public PrerunExpression {
private:
  String value;

  u64 bitWidth;

  bool isUnsigned;

public:
  CustomIntegerLiteral(String _value, bool _isUnsigned, u32 _bitWidth, FileRange _fileRange);

  IR::PrerunValue* emit(IR::Context* ctx) override;

  useit Json   toJson() const override;
  useit String toString() const final;

  useit NodeType nodeType() const override { return NodeType::customIntegerLiteral; }
};

} // namespace qat::ast

#endif