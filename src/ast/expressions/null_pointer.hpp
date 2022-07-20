#ifndef QAT_AST_EXPRESSIONS_NULL_POINTER_HPP
#define QAT_AST_EXPRESSIONS_NULL_POINTER_HPP

#include "../expression.hpp"

namespace qat::ast {

/**
 *  A null pointer
 *
 */
class NullPointer : public Expression {
private:
  // Type of the pointer
  llvm::Type *type;

public:
  NullPointer(utils::FileRange _fileRange);

  NullPointer(llvm::Type *_type, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::nullPointer; }
};

} // namespace qat::ast

#endif