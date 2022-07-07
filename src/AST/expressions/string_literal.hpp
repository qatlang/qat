#ifndef QAT_AST_EXPRESSIONS_STRING_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_STRING_LITERAL_HPP

#include "../../IR/generator.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

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

  llvm::Value *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() { return NodeType::stringLiteral; }

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif