#ifndef QAT_AST_PRERUN_SENTENCE_HPP
#define QAT_AST_PRERUN_SENTENCE_HPP

#include "../emit_ctx.hpp"

namespace qat::ast {

class PrerunSentence {
protected:
  FileRange fileRange;

public:
  PrerunSentence(FileRange _fileRange) : fileRange(_fileRange) {}

  virtual void emit(EmitCtx* ctx) = 0;
};

useit bool emit_prerun_sentences(Vec<PrerunSentence*>& sentences, EmitCtx* ctx);

} // namespace qat::ast

#endif