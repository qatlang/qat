#ifndef QAT_AST_EXPRESSIONS_UNSIGNED_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_UNSIGNED_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat {
namespace AST {

class UnsignedLiteral : public Expression {
private:
  /**
   *  String representation of the integer
   *
   */
  std::string value;

public:
  /**
   *  An Unsigned Integer Literal
   *
   * @param _value String representation of the integer
   * @param _filePlacement
   */
  UnsignedLiteral(std::string _value, utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() { return qat::AST::NodeType::unsignedLiteral; }
};

} // namespace AST
} // namespace qat

#endif