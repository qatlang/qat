#ifndef QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_CONSTANTS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/helpers.hpp"
#include "../expression.hpp"

namespace qat::ast {

class CustomIntegerLiteral : public PrerunExpression, public TypeInferrable {
private:
  String      value;
  Maybe<u32>  bitWidth;
  Maybe<u8>   radix;
  Maybe<bool> isUnsigned;

  Maybe<Identifier> suffix;

  static String radixDigits;
  static String radixDigitsUpper;

public:
  CustomIntegerLiteral(String _value, Maybe<bool> _isUnsigned, Maybe<u32> _bitWidth, Maybe<u8> _radix,
                       Maybe<Identifier> _suffix, FileRange _fileRange)
      : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bitWidth(_bitWidth), radix(_radix),
        isUnsigned(_isUnsigned), suffix(_suffix) {}

  useit static inline CustomIntegerLiteral* create(String _value, Maybe<bool> _isUnsigned, Maybe<u32> _bitWidth,
                                                   Maybe<u8> _radix, Maybe<Identifier> _suffix, FileRange _fileRange) {
    return std::construct_at(OwnNormal(CustomIntegerLiteral), _value, _isUnsigned, _bitWidth, _radix, _suffix,
                             _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  IR::PrerunValue* emit(IR::Context* ctx) override;

  useit static String radixToString(u8 val);

  useit Json   toJson() const override;
  useit String toString() const final;

  useit NodeType nodeType() const override { return NodeType::CUSTOM_INTEGER_LITERAL; }
};

} // namespace qat::ast

#endif