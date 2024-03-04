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

  useit MixOrChoiceMatchValue* asMixOrChoice();
  useit ExpressionMatchValue*  asExp();
  useit virtual FileRange      getMainRange() const = 0;
  useit virtual MatchType      getType() const      = 0;
  useit virtual Json           to_json() const      = 0;
};

class MixOrChoiceMatchValue final : public MatchValue {
private:
  Identifier        name;
  Maybe<Identifier> valueName;
  bool              isVar;

public:
  MixOrChoiceMatchValue(Identifier name, Maybe<Identifier> valueName, bool isVar);
  useit static inline MixOrChoiceMatchValue* create(Identifier name, Maybe<Identifier> valueName, bool isVar) {
    return std::construct_at(OwnNormal(MixOrChoiceMatchValue), name, valueName, isVar);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {}

  useit Identifier get_name() const;
  useit bool       hasValueName() const;
  useit Identifier getValueName() const;
  useit bool       is_variable() const;
  useit MatchType  getType() const final { return MatchType::mixOrChoice; }
  useit FileRange  getMainRange() const final { return name.range; }
  useit Json       to_json() const final;
};

class ExpressionMatchValue final : public MatchValue {
private:
  Expression* exp;

public:
  explicit ExpressionMatchValue(Expression* _exp) : exp(_exp) {}

  useit static inline ExpressionMatchValue* create(Expression* exp) {
    return std::construct_at(OwnNormal(ExpressionMatchValue), exp);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(exp);
  }

  useit Expression* getExpression() const;
  useit MatchType   getType() const final { return MatchType::Exp; }
  useit FileRange   getMainRange() const final { return exp->fileRange; }
  useit Json        to_json() const final;
};

struct CaseResult {
  Maybe<bool> result;
  bool        areAllConstant = false;
  CaseResult(Maybe<bool> result, bool areAllConstant);
};

class Match final : public Sentence {
private:
  bool                                        isTypeMatch;
  Expression*                                 candidate;
  Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> chain;
  Maybe<Pair<Vec<Sentence*>, FileRange>>      elseCase;

  Vec<CaseResult> matchResult;

public:
  Match(bool _isTypeMatch, Expression* _candidate, Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> _chain,
        Maybe<Pair<Vec<Sentence*>, FileRange>> _elseCase, FileRange _fileRange)
      : Sentence(_fileRange), isTypeMatch(_isTypeMatch), candidate(_candidate), chain(_chain), elseCase(_elseCase) {}

  useit static inline Match* create(bool _isTypeMatch, Expression* _candidate,
                                    Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> _chain,
                                    Maybe<Pair<Vec<Sentence*>, FileRange>> _elseCase, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Match), _isTypeMatch, _candidate, _chain, _elseCase, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
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

  useit bool hasConstResultForAllCases();
  useit bool isFalseForAllCases();
  useit bool isTrueForACase();

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::MATCH; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif