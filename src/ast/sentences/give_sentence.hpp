#ifndef QAT_AST_SENTENCES_GIVE_SENTENCE_HPP
#define QAT_AST_SENTENCES_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include <optional>

namespace qat::ast {

class GiveSentence final : public Sentence {
  private:
	Maybe<Expression*> give_expr;

  public:
	GiveSentence(Maybe<Expression*> _given_expr, FileRange _fileRange)
		: Sentence(std::move(_fileRange)), give_expr(_given_expr) {}

	useit static GiveSentence* create(Maybe<Expression*> _given_expr, FileRange _fileRange) {
		return std::construct_at(OwnNormal(GiveSentence), _given_expr, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (give_expr.has_value()) {
			UPDATE_DEPS(give_expr.value());
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) override;
	useit Json		 to_json() const override;
	useit NodeType	 nodeType() const override { return NodeType::GIVE_SENTENCE; }
};

} // namespace qat::ast

#endif
