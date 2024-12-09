#ifndef QAT_AST_META_TODO_HPP
#define QAT_AST_META_TODO_HPP

#include "../prerun_sentences/prerun_sentence.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class PrerunMetaTodo : public PrerunSentence {
	Maybe<String> message;

  public:
	PrerunMetaTodo(Maybe<String> _message, FileRange _fileRange) : PrerunSentence(_fileRange), message(_message) {}

	useit static PrerunMetaTodo* create(Maybe<String> message, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunMetaTodo), message, fileRange);
	}

	void emit(EmitCtx* ctx) final;
};

class MetaTodo : public Sentence {
	Maybe<String> message;

  public:
	MetaTodo(Maybe<String> _message, FileRange _fileRange) : Sentence(_fileRange), message(_message) {}

	useit static MetaTodo* create(Maybe<String> message, FileRange fileRange) {
		return std::construct_at(OwnNormal(MetaTodo), message, fileRange);
	}

	virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
	                                 EmitCtx* ctx) {}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit NodeType   nodeType() const final { return NodeType::META_TODO; }
	useit Json       to_json() const final;
};

} // namespace qat::ast

#endif