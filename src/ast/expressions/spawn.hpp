#ifndef QAT_AST_EXPRESSION_SPAWN_HPP
#define QAT_AST_EXPRESSION_SPAWN_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class Spawn : public Expression {
	Vec<Sentence*> sentences;

  public:
	Spawn(Vec<Sentence*> _sentences, FileRange _fileRange) : Expression(_fileRange), sentences(_sentences) {}

	useit static Spawn* create(Vec<Sentence*> sentences, FileRange fileRange) {
		return std::construct_at(OwnNormal(Spawn), sentences, fileRange);
	}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit Json		 to_json() const final;
	useit NodeType	 nodeType() const final { return NodeType::SPAWN; }
};

} // namespace qat::ast

#endif
