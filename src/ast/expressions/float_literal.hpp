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
  std::string value;

public:
  /**
   *  A Float Literal
   *
   * @param _value String representation of the floating point number
   * @param _fileRange
   */
  FloatLiteral(std::string _value, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::floatLiteral; }
};

} // namespace qat::ast

#endif