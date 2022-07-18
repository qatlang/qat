#ifndef QAT_AST_EXPRESSIONS_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

class CustomFloatLiteral : public Expression {
private:
  // Numerical value of the float
  std::string value;

  // Kind of the float
  std::string kind;

public:
  CustomFloatLiteral(std::string _value, std::string _kind,
                     utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::customFloatLiteral; }
};

} // namespace qat::ast

#endif