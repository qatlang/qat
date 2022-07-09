#ifndef QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP
#define QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP

#include "../expression.hpp"

namespace qat {
namespace AST {

class LoopIndex : public Expression {
public:
  LoopIndex(utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::loopIndex; }
};

} // namespace AST
} // namespace qat

#endif