#ifndef QAT_AST_SENTENCE_HPP
#define QAT_AST_SENTENCE_HPP

#include "./node.hpp"
#include "emit_ctx.hpp"

namespace qat::ast {

class Sentence : public Node {
public:
  Sentence(FileRange _fileRange) : Node(std::move(_fileRange)) {}

  virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                   EmitCtx* ctx) = 0;

  useit virtual ir::Value* emit(EmitCtx* ctx)        = 0;
  useit NodeType           nodeType() const override = 0;
  useit Json               to_json() const override  = 0;
  ~Sentence() override                               = default;
};

void emit_sentences(const Vec<Sentence*>& sentences, EmitCtx* ctx);

} // namespace qat::ast

#endif