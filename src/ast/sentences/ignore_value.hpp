#ifndef QAT_AST_SENTENCES_IGNORE_VALUE_HPP
#define QAT_AST_SENTENCES_IGNORE_VALUE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Expression;

class IgnoreValue : public Sentence {
	Expression* candidate;

  public:
	IgnoreValue(Expression* _candidate, FileRangePtr _fileRange) : Sentence(_fileRange), candidate(_candidate) {}

	useit static IgnoreValue* create(Expression* candidate, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(IgnoreValue), candidate, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx);

	useit ir::Value* emit(EmitCtx* ctx);

	useit NodeType nodeType() const final { return NodeType::IGNORE_VALUE; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
