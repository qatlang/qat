#ifndef QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat {
namespace AST {

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
  FloatLiteral(std::string _value, utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return qat::AST::NodeType::floatLiteral; }
};

} // namespace AST
} // namespace qat

#endif