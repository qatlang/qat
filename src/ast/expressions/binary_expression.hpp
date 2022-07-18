#ifndef QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP
#define QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

class BinaryExpression : public Expression {
private:
  std::string op;
  Expression *lhs;
  Expression *rhs;

public:
  BinaryExpression(Expression *_lhs, std::string _binaryOperator,
                   Expression *_rhs, utils::FileRange _fileRange)
      : lhs(_lhs), op(_binaryOperator), rhs(_rhs), Expression(_fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::binaryExpression; }
};

} // namespace qat::ast

#endif