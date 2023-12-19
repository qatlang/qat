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

  useit MixOrChoiceMatchValue* asMixOrChoice();
  useit ExpressionMatchValue*  asExp();
  useit virtual FileRange      getMainRange() const = 0;
  useit virtual MatchType      getType() const      = 0;
  useit virtual Json           toJson() const       = 0;
};

class MixOrChoiceMatchValue : public MatchValue {
private:
  Identifier        name;
  Maybe<Identifier> valueName;
  bool              isVar;

public:
  MixOrChoiceMatchValue(Identifier name, Maybe<Identifier> valueName, bool isVar);

  useit Identifier getName() const;
  useit bool       hasValueName() const;
  useit Identifier getValueName() const;
  useit bool       isVariable() const;
  useit MatchType  getType() const final { return MatchType::mixOrChoice; }
  useit FileRange  getMainRange() const final { return name.range; }
  useit Json       toJson() const final;
};

class ExpressionMatchValue : public MatchValue {
private:
  Expression* exp;

public:
  explicit ExpressionMatchValue(Expression* exp);

  useit Expression* getExpression() const;
  useit MatchType   getType() const final { return MatchType::Exp; }
  useit FileRange   getMainRange() const final { return exp->fileRange; }
  useit Json        toJson() const final;
};

struct CaseResult {
  Maybe<bool> result;
  bool        areAllConstant = false;
  CaseResult(Maybe<bool> result, bool areAllConstant);
};

class Match : public Sentence {
private:
  bool                                        isTypeMatch;
  Expression*                                 candidate;
  Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> chain;
  Maybe<Pair<Vec<Sentence*>, FileRange>>      elseCase;

  Vec<CaseResult> matchResult;

public:
  Match(bool _isTypeMatch, Expression* candidate, Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> chain,
        Maybe<Pair<Vec<Sentence*>, FileRange>> elseCase, FileRange fileRange);

  useit bool hasConstResultForAllCases();
  useit bool isFalseForAllCases();
  useit bool isTrueForACase();

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::match; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif