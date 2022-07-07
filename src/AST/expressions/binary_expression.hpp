#ifndef QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP
#define QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP

#include "../../utils/llvm_type_to_name.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "llvm/IR/Value.h"
#include <string>

namespace qat {
namespace AST {
class BinaryExpression : public Expression {
private:
  std::string op;
  Expression *lhs;
  Expression *rhs;

public:
  BinaryExpression(Expression *_lhs, std::string _binaryOperator,
                   Expression *_rhs, utils::FilePlacement _filePlacement)
      : lhs(_lhs), op(_binaryOperator), rhs(_rhs), Expression(_filePlacement) {}

  llvm::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::binaryExpression; }
};
} // namespace AST
} // namespace qat

#endif