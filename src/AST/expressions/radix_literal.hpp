#ifndef QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_RADIX_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::AST {

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
   * @param _filePlacement
   */
  RadixLiteral(std::string _value, unsigned _radix,
               utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return qat::AST::NodeType::radixLiteral; }
};

} // namespace qat::AST

#endif