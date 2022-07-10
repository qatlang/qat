#ifndef QAT_AST_EXPRESSIONS_STRING_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_STRING_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat {
namespace AST {

/**
 *  StringLiteral is used to represent literal strings
 *
 */
class StringLiteral : public Expression {
private:
  /**
   *  Value of the string
   *
   */
  std::string value;

public:
  /**
   *  StringLiteral is used to represent literal strings
   *
   * @param _value Value of the string
   * @param _filePlacement
   */
  StringLiteral(std::string _value, utils::FilePlacement _filePlacement);

  /**
   *  Get the value of the string
   *
   * @return std::string Value of the string
   */
  std::string get_value() const;

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() { return NodeType::stringLiteral; }

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif