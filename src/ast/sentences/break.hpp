#ifndef QAT_AST_SENTENCES_BREAK_HPP
#define QAT_AST_SENTENCES_BREAK_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Break final : public Sentence {
	Maybe<Identifier> tag;

  public:
	Break(Maybe<Identifier> _tag, FileRangePtr _fileRange) : Sentence(_fileRange), tag(_tag) {}

	static Break* create(Maybe<Identifier> _tag, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(Break), _tag, _fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::BREAK; }
};

} // namespace qat::ast

#endif
