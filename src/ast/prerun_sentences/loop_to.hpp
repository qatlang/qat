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
	PrerunLoopTo(PrerunExpression* _count, Vec<PrerunSentence*> _sentences, FileRange _fileRange)
	    : PrerunSentence(_fileRange), count(_count), sentences(std::move(_sentences)) {}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
		UPDATE_DEPS(count);
		for (auto* snt : sentences) {
			UPDATE_DEPS(snt);
		}
	}

	useit static PrerunLoopTo* create(PrerunExpression* count, Vec<PrerunSentence*> sentences, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunLoopTo), count, std::move(sentences), fileRange);
	}

	void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
