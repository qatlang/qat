#ifndef QAT_AST_EXPRESSION_SPAWN_HPP
#define QAT_AST_EXPRESSION_SPAWN_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class Spawn : public Expression {
	Vec<Sentence*> sentences;

  public:
	Spawn(Vec<Sentence*> _sentences, FileRangePtr _fileRange) : Expression(_fileRange), sentences(_sentences) {}

	static Spawn* create(Vec<Sentence*> sentences, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Spawn), sentences, fileRange);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::SPAWN; }
};

} // namespace qat::ast

#endif
