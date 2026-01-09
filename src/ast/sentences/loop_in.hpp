#ifndef QAT_AST_SENTENCES_LOOP_IN_HPP
#define QAT_AST_SENTENCES_LOOP_IN_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class LoopIn : public Sentence {
	Expression*    candidate;
	Vec<Sentence*> sentences;
	Identifier     itemName;

	Maybe<Identifier> indexName;

  public:
	LoopIn(Expression* _candidate, Vec<Sentence*> _sentences, Identifier _itemName, Maybe<Identifier> _indexName,
	       FileRangePtr _fileRange)
	    : Sentence(std::move(_fileRange)), candidate(_candidate), sentences(std::move(_sentences)),
	      itemName(std::move(_itemName)), indexName(std::move(_indexName)) {}

	static LoopIn* create(Expression* candidate, Vec<Sentence*> sentences, Identifier itemName,
	                      Maybe<Identifier> indexName, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(LoopIn), candidate, std::move(sentences), std::move(itemName),
		                         std::move(indexName), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
		UPDATE_DEPS(candidate);
		for (auto snt : sentences) {
			UPDATE_DEPS(snt);
		}
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const { return NodeType::LOOP_OVER; }
};

} // namespace qat::ast

#endif
