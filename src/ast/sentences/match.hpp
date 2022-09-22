#ifndef QAT_AST_MATCH_HPP
#define QAT_AST_MATCH_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

enum class MatchType { Union, Enum, Exp };

class UnionMatchValue;
class EnumMatchValue;
class ExpressionMatchValue;

class MatchValue {
public:
  ~MatchValue() = default;

  useit UnionMatchValue      *asUnion();
  useit EnumMatchValue       *asEnum();
  useit ExpressionMatchValue *asExp();
  useit virtual MatchType     getType() const = 0;
  useit virtual nuo::Json     toJson() const  = 0;
};

class UnionMatchValue : public MatchValue {
private:
  Pair<String, utils::FileRange>        name;
  Maybe<Pair<String, utils::FileRange>> valueName;
  bool                                  isVar;

public:
  UnionMatchValue(Pair<String, utils::FileRange>        name,
                  Maybe<Pair<String, utils::FileRange>> valueName, bool isVar);

  useit String getName() const;
  useit utils::FileRange getNameRange() const;
  useit bool             hasValueName() const;
  useit String           getValueName() const;
  useit utils::FileRange getValueRange() const;
  useit bool             isVariable() const;
  useit MatchType        getType() const final { return MatchType::Union; }
  useit nuo::Json toJson() const final;
};

class EnumMatchValue : public MatchValue {
private:
  Pair<String, utils::FileRange> name;

public:
  EnumMatchValue(Pair<String, utils::FileRange> name);

  useit String getName() const;
  useit utils::FileRange getFileRange() const;
  useit MatchType        getType() const final { return MatchType::Enum; }
  useit nuo::Json toJson() const final;
};

class ExpressionMatchValue : public MatchValue {
private:
  Expression *exp;

public:
  useit Expression *getExpression() const;
  useit MatchType   getType() const final { return MatchType::Exp; }
  useit nuo::Json toJson() const final;
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
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif