#ifndef QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral : public Expression {
private:
  std::string value;

public:
  IntegerLiteral(std::string _value, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() { return NodeType::integerLiteral; }
};

} // namespace qat::ast

#endif