#ifndef QAT_AST_SENTENCES_GIVE_SENTENCE_HPP
#define QAT_AST_SENTENCES_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class GiveSentence final : public Sentence {
  private:
	Expression* value;

  public:
	GiveSentence(Expression* _value, FileRangePtr _fileRange) : Sentence(_fileRange), value(_value) {}

	useit static GiveSentence* create(Expression* _value, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(GiveSentence), _value, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		if (value) {
			UPDATE_DEPS(value);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) override;

	useit NodeType nodeType() const override { return NodeType::GIVE_SENTENCE; }
};

} // namespace qat::ast

#endif
