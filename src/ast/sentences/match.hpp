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

  useit MixMatchValue           *asMix();
  useit ChoiceMatchValue        *asChoice();
  useit ExpressionMatchValue    *asExp();
  useit virtual utils::FileRange getMainRange() const = 0;
  useit virtual MatchType        getType() const      = 0;
  useit virtual Json             toJson() const       = 0;
};

class MixMatchValue : public MatchValue {
private:
  Pair<String, utils::FileRange>        name;
  Maybe<Pair<String, utils::FileRange>> valueName;
  bool                                  isVar;

public:
  MixMatchValue(Pair<String, utils::FileRange>        name,
                Maybe<Pair<String, utils::FileRange>> valueName, bool isVar);

  useit String getName() const;
  useit utils::FileRange getNameRange() const;
  useit bool             hasValueName() const;
  useit String           getValueName() const;
  useit utils::FileRange getValueRange() const;
  useit bool             isVariable() const;
  useit MatchType        getType() const final { return MatchType::mix; }
  useit utils::FileRange getMainRange() const final { return name.second; }
  useit Json             toJson() const final;
};

class ChoiceMatchValue : public MatchValue {
private:
  String           name;
  utils::FileRange range;

public:
  ChoiceMatchValue(String name, utils::FileRange fileRange);

  useit String getName() const;
  useit utils::FileRange getFileRange() const;
  useit MatchType        getType() const final { return MatchType::choice; }
  useit utils::FileRange getMainRange() const final { return range; }
  useit Json             toJson() const final;
};

class ExpressionMatchValue : public MatchValue {
private:
  Expression *exp;

public:
  ExpressionMatchValue(Expression *exp);

  useit Expression *getExpression() const;
  useit MatchType   getType() const final { return MatchType::Exp; }
  useit utils::FileRange getMainRange() const final { return exp->fileRange; }
  useit Json             toJson() const final;
};

class Match : public Sentence {
private:
  bool                                     isTypeMatch;
  Expression                              *candidate;
  Vec<Pair<MatchValue *, Vec<Sentence *>>> chain;
  Maybe<Vec<Sentence *>>                   elseCase;

public:
  Match(bool _isTypeMatch, Expression *candidate,
        Vec<Pair<MatchValue *, Vec<Sentence *>>> chain,
        Maybe<Vec<Sentence *>> elseCase, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::match; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif