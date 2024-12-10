#ifndef QAT_AST_PRERUN_SENTENCES_EXPRESSION_SENTENCE_HPP
#define QAT_AST_PRERUN_SENTENCES_EXPRESSION_SENTENCE_HPP

#include "./prerun_sentence.hpp"

namespace qat::ast {

class PrerunExpressionSentence : public PrerunSentence {
	PrerunExpression* expression;

  public:
	PrerunExpressionSentence(PrerunExpression* _exp, FileRange _fileRange)
	    : PrerunSentence(std::move(_fileRange)), expression(_exp) {}

	useit static PrerunExpressionSentence* create(PrerunExpression* expression, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunExpressionSentence), expression, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx);

	void emit(EmitCtx* ctx);
};

} // namespace qat::ast

#endif
