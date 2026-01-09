#ifndef QAT_AST_SENTENCES_EXPRESSION_SENTENCE_HPP
#define QAT_AST_SENTENCES_EXPRESSION_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class ExpressionSentence final : public Sentence {
	Expression* expr;

  public:
	ExpressionSentence(Expression* _expr, FileRangePtr _fileRange) : Sentence(_fileRange), expr(_expr) {}

	static ExpressionSentence* create(Expression* _expr, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ExpressionSentence), _expr, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(expr);
	}

	ir::Value* emit(EmitCtx* ctx);

	NodeType nodeType() const { return NodeType::EXPRESSION_SENTENCE; }
};

} // namespace qat::ast

#endif
