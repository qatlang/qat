#ifndef QAT_AST_SENTENCES_LOOP_TO_HPP
#define QAT_AST_SENTENCES_LOOP_TO_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class LoopTo final : public Sentence {
	Vec<Sentence*>    sentences;
	Expression*       count;
	Maybe<Identifier> tag;

  public:
	LoopTo(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag, FileRangePtr _fileRange)
	    : Sentence(_fileRange), sentences(_snts), count(_count), tag(_tag) {}

	static LoopTo* create(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(LoopTo), _count, _snts, _tag, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(count);
		for (auto snt : sentences) {
			UPDATE_DEPS(snt);
		}
	}

	bool       hasTag() const;
	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::LOOP_N_TIMES; }
};

} // namespace qat::ast

#endif
