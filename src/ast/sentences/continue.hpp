#ifndef QAT_AST_SENTENCES_CONTINUE_HPP
#define QAT_AST_SENTENCES_CONTINUE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Continue final : public Sentence {
	Maybe<Identifier> tag;

  public:
	Continue(Maybe<Identifier> _tag, FileRangePtr _fileRange) : Sentence(_fileRange), tag(_tag) {}

	static Continue* create(Maybe<Identifier> _tag, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(Continue), _tag, _fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::CONTINUE; }
};

} // namespace qat::ast

#endif
