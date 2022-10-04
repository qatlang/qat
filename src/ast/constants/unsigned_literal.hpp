#ifndef QAT_AST_CONSTANTS_UNSIGNED_LITERAL_HPP
#define QAT_AST_CONSTANTS_UNSIGNED_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

class UnsignedLiteral : public Expression {
private:
  /**
   *  String representation of the integer
   *
   */
  String value;

public:
  /**
   *  An Unsigned Integer Literal
   *
   * @param _value String representation of the integer
   * @param _fileRange
   */
  UnsignedLiteral(String _value, utils::FileRange _fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit NodeType           nodeType() const override { return NodeType::unsignedLiteral; }
};

} // namespace qat::ast

#endif