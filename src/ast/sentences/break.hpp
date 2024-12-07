#ifndef QAT_AST_SENTENCES_BREAK_HPP
#define QAT_AST_SENTENCES_BREAK_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Break final : public Sentence {
	Maybe<Identifier> tag;

  public:
	Break(Maybe<Identifier> _tag, FileRange _fileRange) : Sentence(_fileRange), tag(_tag) {}

	useit static Break* create(Maybe<Identifier> _tag, FileRange _fileRange) {
		return std::construct_at(OwnNormal(Break), _tag, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
	}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit NodeType	 nodeType() const final { return NodeType::BREAK; }
	useit Json		 to_json() const final;
};

} // namespace qat::ast

#endif
