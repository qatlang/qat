#ifndef QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

class RadixLiteral : public Expression {
private:
  // String representation of the integer
  String value;

  // Radix value
  u64 radix;

public:
  /**
   *  A radix integer Literal
   *
   * @param _value String representation of the integer
   * @param _radix Radix value
   * @param _fileRange
   */
  RadixLiteral(String _value, u64 _radix, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::radixLiteral; }
};

} // namespace qat::ast

#endif