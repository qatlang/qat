#ifndef QAT_AST_PRERUN_LOOP_TO_HPP
#define QAT_AST_PRERUN_LOOP_TO_HPP

#include "../expression.hpp"
#include "./prerun_sentence.hpp"

namespace qat::ast {

class PrerunLoopTo final : public PrerunSentence {
  PrerunExpression*    count;
  Maybe<Identifier>    tag;
  Vec<PrerunSentence*> sentences;

public:
  PrerunLoopTo(PrerunExpression* _count, FileRange _fileRange) : PrerunSentence(_fileRange), count(_count) {}

  useit static inline PrerunLoopTo* get(PrerunExpression* count, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunLoopTo), count, fileRange);
  }

  void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
