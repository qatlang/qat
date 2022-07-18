#ifndef QAT_AST_EXPRESSIONS_NULL_POINTER_HPP
#define QAT_AST_EXPRESSIONS_NULL_POINTER_HPP

#include "../expression.hpp"

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

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::nullPointer; }
};

} // namespace AST
} // namespace qat

#endif