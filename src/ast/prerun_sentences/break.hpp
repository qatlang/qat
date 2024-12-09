#ifndef QAT_AST_PRERUN_SENTENCES_BREAK_HPP
#define QAT_AST_PRERUN_SENTENCES_BREAK_HPP

#include "prerun_sentence.hpp"

namespace qat::ast {

class PrerunBreak final : public PrerunSentence {
	Maybe<Identifier> tag;

  public:
	PrerunBreak(Maybe<Identifier> _tag, FileRange _fileRange)
	    : PrerunSentence(std::move(_fileRange)), tag(std::move(_tag)) {}

	useit static PrerunBreak* create(Maybe<Identifier> tag, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunBreak), std::move(tag), std::move(fileRange));
	}

	void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
