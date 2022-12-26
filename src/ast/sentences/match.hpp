#ifndef QAT_AST_MATCH_HPP
#define QAT_AST_MATCH_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

enum class MatchType { mix, choice, Exp };

class MixMatchValue;
class ChoiceMatchValue;
class ExpressionMatchValue;

class MatchValue {
public:
  ~MatchValue() = default;

  useit MixMatchValue*        asMix();
  useit ChoiceMatchValue*     asChoice();
  useit ExpressionMatchValue* asExp();
  useit virtual FileRange     getMainRange() const = 0;
  useit virtual MatchType     getType() const      = 0;
  useit virtual Json          toJson() const       = 0;
};

class MixMatchValue : public MatchValue {
private:
  Identifier        name;
  Maybe<Identifier> valueName;
  bool              isVar;

public:
  MixMatchValue(Identifier name, Maybe<Identifier> valueName, bool isVar);

  useit Identifier getName() const;
  useit bool       hasValueName() const;
  useit Identifier getValueName() const;
  useit bool       isVariable() const;
  useit MatchType  getType() const final { return MatchType::mix; }
  useit FileRange  getMainRange() const final { return name.range; }
  useit Json       toJson() const final;
};

class ChoiceMatchValue : public MatchValue {
private:
  Identifier name;

public:
  explicit ChoiceMatchValue(Identifier name);

  useit Identifier getName() const;
  useit FileRange  getFileRange() const;
  useit MatchType  getType() const final { return MatchType::choice; }
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

class Match : public Sentence {
private:
  bool                                        isTypeMatch;
  Expression*                                 candidate;
  Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> chain;
  Maybe<Pair<Vec<Sentence*>, FileRange>>      elseCase;

public:
  Match(bool _isTypeMatch, Expression* candidate, Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> chain,
        Maybe<Pair<Vec<Sentence*>, FileRange>> elseCase, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::match; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif