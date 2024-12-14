#ifndef QAT_AST_PRERUN_SENTENCES_IF_ELSE_HPP
#define QAT_AST_PRERUN_SENTENCES_IF_ELSE_HPP

#include "./prerun_sentence.hpp"

namespace qat::ast {

class PrerunIfElse final : public PrerunSentence {
	Pair<PrerunExpression*, Vec<PrerunSentence*>>      ifBlock;
	Vec<Pair<PrerunExpression*, Vec<PrerunSentence*>>> elseIfChain;
	Maybe<Vec<PrerunSentence*>>                        elseBlock;

  public:
	PrerunIfElse(Pair<PrerunExpression*, Vec<PrerunSentence*>>      _ifBlock,
	             Vec<Pair<PrerunExpression*, Vec<PrerunSentence*>>> _elseIfChain,
	             Maybe<Vec<PrerunSentence*>> _elseBlock, FileRange _fileRange)
	    : PrerunSentence(std::move(_fileRange)), ifBlock(std::move(_ifBlock)), elseIfChain(std::move(_elseIfChain)),
	      elseBlock(std::move(_elseBlock)) {}

	useit static PrerunIfElse* create(Pair<PrerunExpression*, Vec<PrerunSentence*>>      ifBlock,
	                                  Vec<Pair<PrerunExpression*, Vec<PrerunSentence*>>> elseIfChain,
	                                  Maybe<Vec<PrerunSentence*>> elseBlock, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunIfElse), std::move(ifBlock), std::move(elseIfChain),
		                         std::move(elseBlock), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	void emit(EmitCtx* ctx) final;
};

} // namespace qat::ast

#endif
