#ifndef QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::AST {

class IntegerLiteral : public Expression {
private:
  std::string value;

public:
  IntegerLiteral(std::string _value, utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() { return qat::AST::NodeType::integerLiteral; }
};

} // namespace qat::AST

#endif