#ifndef QAT_AST_PRERUN_SENTENCE_CONTINUE_HPP
#define QAT_AST_PRERUN_SENTENCE_CONTINUE_HPP

#include "./prerun_sentence.hpp"

namespace qat::ast {

class PrerunContinue : public PrerunSentence {
  Maybe<Identifier> tag;

public:
  PrerunContinue(Maybe<Identifier> _tag, FileRange _fileRange)
      : PrerunSentence(std::move(_fileRange)), tag(std::move(_tag)) {}

  useit static PrerunContinue* create(Maybe<Identifier> tag, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunContinue), std::move(tag), std::move(fileRange));
  }

  void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
