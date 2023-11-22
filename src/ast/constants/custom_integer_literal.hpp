#ifndef QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/helpers.hpp"
#include "../expression.hpp"

namespace qat::ast {

class CustomIntegerLiteral : public PrerunExpression {
private:
  String value;

  Maybe<u32>  bitWidth;
  Maybe<u8>   radix;
  Maybe<bool> isUnsigned;

  static String radixDigits;
  static String radixDigitsUpper;

public:
  CustomIntegerLiteral(String _value, Maybe<bool> _isUnsigned, Maybe<u32> _bitWidth, Maybe<u8> _radix,
                       FileRange _fileRange);

  IR::PrerunValue* emit(IR::Context* ctx) override;

  useit static String radixToString(u8 val);

  useit Json   toJson() const override;
  useit String toString() const final;

  useit NodeType nodeType() const override { return NodeType::customIntegerLiteral; }
};

} // namespace qat::ast

#endif