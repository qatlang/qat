#ifndef QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP
#define QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP

#include "../expression.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include <vector>

namespace qat {
namespace AST {
class TupleValue : public Expression {
  std::vector<Expression *> members;

public:
  TupleValue(std::vector<Expression *> _members,
             utils::FilePlacement _filePlacement);

  llvm::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::tupleValue; }
};
} // namespace AST
} // namespace qat

#endif