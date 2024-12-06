#ifndef QAT_AST_PRERUN_GIVE_SENTENCE_HPP
#define QAT_AST_PRERUN_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "./prerun_sentence.hpp"

namespace qat::ast {

class PrerunGive final : public PrerunSentence {
  Maybe<PrerunExpression*> value;

public:
  PrerunGive(Maybe<PrerunExpression*> _value, FileRange _fileRange) : PrerunSentence(_fileRange), value(_value) {}

  useit static PrerunGive* get(Maybe<PrerunExpression*> value, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunGive), value, fileRange);
  }

  void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
