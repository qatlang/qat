#ifndef QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

class RadixLiteral : public Expression {
private:
  // String representation of the integer
  std::string value;

  // Radix value
  unsigned radix;

public:
  /**
   *  A radix integer Literal
   *
   * @param _value String representation of the integer
   * @param _radix Radix value
   * @param _fileRange
   */
  RadixLiteral(std::string _value, unsigned _radix,
               utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::radixLiteral; }
};

} // namespace qat::ast

#endif