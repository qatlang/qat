#ifndef QAT_AST_MATCH_HPP
#define QAT_AST_MATCH_HPP

#include "../../utils/file_range.hpp"
#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

enum class MatchType { mixOrChoice, Exp };

class MixOrChoiceMatchValue;
class ExpressionMatchValue;

class MatchValue {
  public:
	~MatchValue() = default;

	virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
	                                 EmitCtx* ctx) = 0;

	MixOrChoiceMatchValue* asMixOrChoice();

	ExpressionMatchValue* asExp();

	virtual FileRangePtr getMainRange() const = 0;

	virtual MatchType getType() const = 0;
};

class MixOrChoiceMatchValue final : public MatchValue {
  private:
	Identifier        name;
	Maybe<Identifier> valueName;
	bool              isVar;

  public:
	MixOrChoiceMatchValue(Identifier name, Maybe<Identifier> valueName, bool isVar);

	static MixOrChoiceMatchValue* create(Identifier name, Maybe<Identifier> valueName, bool isVar) {
		return std::construct_at(OwnNormal(MixOrChoiceMatchValue), name, valueName, isVar);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	Identifier get_name() const;
	bool       hasValueName() const;
	Identifier getValueName() const;
	bool       is_variable() const;

	MatchType getType() const final { return MatchType::mixOrChoice; }

	FileRangePtr getMainRange() const final { return name.range; }
};

class ExpressionMatchValue final : public MatchValue {
  private:
	Expression* exp;

  public:
	explicit ExpressionMatchValue(Expression* _exp) : exp(_exp) {}

	static ExpressionMatchValue* create(Expression* exp) {
		return std::construct_at(OwnNormal(ExpressionMatchValue), exp);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> _, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(exp);
	}

	Expression* getExpression() const;

	MatchType getType() const final { return MatchType::Exp; }

	FileRangePtr getMainRange() const final { return exp->fileRange; }
};

struct CaseResult {
	Maybe<bool> result;
	bool        areAllConstant = false;
	CaseResult(Maybe<bool> result, bool areAllConstant);
};

class Match final : public Sentence {
  private:
	Expression*                                 candidate;
	Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> chain;
	Maybe<Pair<Vec<Sentence*>, FileRangePtr>>   elseCase;

	Vec<CaseResult> matchResult;

  public:
	Match(Expression* _candidate, Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> _chain,
	      Maybe<Pair<Vec<Sentence*>, FileRangePtr>> _elseCase, FileRangePtr _fileRange)
	    : Sentence(_fileRange), candidate(_candidate), chain(_chain), elseCase(_elseCase) {}

	static Match* create(Expression* _candidate, Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> _chain,
	                     Maybe<Pair<Vec<Sentence*>, FileRangePtr>> _elseCase, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(Match), _candidate, _chain, _elseCase, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> _, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
		for (auto& ch : chain) {
			for (auto m : ch.first) {
				UPDATE_DEPS(m);
			}
			for (auto snt : ch.second) {
				UPDATE_DEPS(snt);
			}
		}
		if (elseCase.has_value()) {
			for (auto snt : elseCase.value().first) {
				UPDATE_DEPS(snt);
			}
		}
	}

	bool hasConstResultForAllCases();
	bool isFalseForAllCases();
	bool isTrueForACase();

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::MATCH; }
};

} // namespace qat::ast

#endif
