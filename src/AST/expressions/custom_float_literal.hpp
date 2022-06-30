#ifndef QAT_AST_EXPRESSIONS_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"
#include <string>

namespace qat {
namespace AST {

class CustomFloatLiteral : public Expression {
private:
  // Numerical value of the float
  std::string value;

  // Kind of the float
  std::string kind;

public:
  CustomFloatLiteral(std::string _value, std::string _kind,
                     utils::FilePlacement _filePlacement);

  llvm::Value *generate(IR::Generator *generator);

  backend::JSON toJSON() const;

  NodeType nodeType() { return NodeType::customFloatLiteral; }
};

} // namespace AST
} // namespace qat

#endif