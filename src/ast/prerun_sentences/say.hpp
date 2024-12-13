#ifndef QAT_AST_PRERUN_SENTENCES_SAY_HPP
#define QAT_AST_PRERUN_SENTENCES_SAY_HPP

#include "./prerun_sentence.hpp"

namespace qat::ast {

class PrerunSay : public PrerunSentence {
	Vec<PrerunExpression*> values;

  public:
	PrerunSay(Vec<PrerunExpression*> _values, FileRange _fileRange)
	    : PrerunSentence(std::move(_fileRange)), values(std::move(_values)) {}

	useit static PrerunSay* create(Vec<PrerunExpression*> values, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunSay), std::move(values), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
