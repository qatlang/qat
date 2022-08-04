#ifndef QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FloatLiteral : public Expression {
private:
  /**
   *  String representation of the floating point number
   *
   */
  String value;

public:
  /**
   *  A Float Literal
   *
   * @param _value String representation of the floating point number
   * @param _fileRange
   */
  FloatLiteral(String _value, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::floatLiteral; }
};

} // namespace qat::ast

#endif