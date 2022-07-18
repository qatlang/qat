#ifndef QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP
#define QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP

#include "../expression.hpp"

namespace qat::AST {

class LoopIndex : public Expression {
public:
  LoopIndex(utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::loopIndex; }
};

} // namespace qat::AST

#endif