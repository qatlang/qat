#ifndef QAT_AST_META_TODO_HPP
#define QAT_AST_META_TODO_HPP

#include "../prerun_sentences/prerun_sentence.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class PrerunMetaTodo : public PrerunSentence {
	Maybe<String> message;

  public:
	PrerunMetaTodo(Maybe<String> _message, FileRangePtr _fileRange) : PrerunSentence(_fileRange), message(_message) {}

	useit static PrerunMetaTodo* create(Maybe<String> message, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunMetaTodo), message, fileRange);
	}

	void emit(EmitCtx* ctx) final;
};

class MetaTodo : public Sentence {
	Maybe<String> message;

  public:
	MetaTodo(Maybe<String> _message, FileRangePtr _fileRange) : Sentence(_fileRange), message(_message) {}

	useit static MetaTodo* create(Maybe<String> message, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(MetaTodo), message, fileRange);
	}

	virtual void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) {}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::META_TODO; }
};

} // namespace qat::ast

#endif
