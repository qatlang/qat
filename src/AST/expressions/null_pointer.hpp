#ifndef QAT_AST_EXPRESSIONS_NULL_POINTER_HPP
#define QAT_AST_EXPRESSIONS_NULL_POINTER_HPP

#include "../expression.hpp"
#include "llvm/IR/Type.h"

namespace qat {
namespace AST {

/**
 *  A null pointer
 *
 */
class NullPointer : public Expression {
private:
  // Type of the pointer
  llvm::Type *type;

public:
  NullPointer(utils::FilePlacement _filePlacement);

  NullPointer(llvm::Type *_type, utils::FilePlacement _filePlacement);

  llvm::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::nullPointer; }
};

} // namespace AST
} // namespace qat

#endif