#ifndef QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::AST {

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
   * @param _filePlacement
   */
  FloatLiteral(std::string _value, utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return qat::AST::NodeType::floatLiteral; }
};

} // namespace qat::AST

#endif