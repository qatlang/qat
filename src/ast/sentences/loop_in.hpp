#ifndef QAT_AST_SENTENCES_LOOP_IN_HPP
#define QAT_AST_SENTENCES_LOOP_IN_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class LoopIn : public Sentence {
	Expression*	   candidate;
	Vec<Sentence*> sentences;
	Identifier	   itemName;

	Maybe<Identifier> indexName;

  public:
	LoopIn(Expression* _candidate, Vec<Sentence*> _sentences, Identifier _itemName, Maybe<Identifier> _indexName,
		   FileRange _fileRange)
		: Sentence(std::move(_fileRange)), candidate(_candidate), sentences(std::move(_sentences)),
		  itemName(std::move(_itemName)), indexName(std::move(_indexName)) {}

	useit static LoopIn* create(Expression* candidate, Vec<Sentence*> sentences, Identifier itemName,
								Maybe<Identifier> indexName, FileRange fileRange) {
		return std::construct_at(OwnNormal(LoopIn), candidate, std::move(sentences), std::move(itemName),
								 std::move(indexName), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
		UPDATE_DEPS(candidate);
		for (auto snt : sentences) {
			UPDATE_DEPS(snt);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit Json to_json() const final {
		Vec<JsonValue> sentencesJSON;
		for (auto* snt : sentences) {
			sentencesJSON.push_back(snt->to_json());
		}
		return Json()
			._("itemName", itemName)
			._("hasIndexName", indexName.has_value())
			._("sentences", sentencesJSON)
			._("fileRange", fileRange);
	}

	useit NodeType nodeType() const { return NodeType::LOOP_OVER; }
};

} // namespace qat::ast

#endif
