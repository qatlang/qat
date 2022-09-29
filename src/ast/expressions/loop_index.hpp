#ifndef QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP
#define QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP

#include "../expression.hpp"

namespace qat::ast {

class LoopIndex : public Expression {
private:
  String indexName;

public:
  LoopIndex(String indexName, utils::FileRange _fileRange);

  useit bool hasName() const;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::loopIndex; }
};

} // namespace qat::ast

#endif