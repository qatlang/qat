#ifndef QAT_AST_SENTENCES_SAY_HPP
#define QAT_AST_SENTENCES_SAY_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

enum class SayType { say, dbg, only };

class SayLike final : public Sentence {
  private:
	Vec<Expression*> expressions;
	SayType			 sayType;

  public:
	SayLike(SayType _sayTy, Vec<Expression*> _expressions, FileRange _fileRange)
		: Sentence(_fileRange), expressions(_expressions), sayType(_sayTy) {}

	useit static SayLike* create(SayType _sayTy, Vec<Expression*> _expressions, FileRange _fileRange) {
		return std::construct_at(OwnNormal(SayLike), _sayTy, _expressions, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto exp : expressions) {
			UPDATE_DEPS(exp);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit NodeType	 nodeType() const final { return NodeType::SAY_LIKE; }
	useit Json		 to_json() const final;
};

} // namespace qat::ast

#endif
