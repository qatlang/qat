#ifndef QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP
#define QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP

#include "../expression.hpp"

namespace qat::ast {

class LoopIndex : public Expression {
public:
  LoopIndex(utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::loopIndex; }
};

} // namespace qat::ast

#endif