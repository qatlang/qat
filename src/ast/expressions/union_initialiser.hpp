#ifndef QAT_AST_EXPRESSIONS_UNION_INITIALISER_HPP
#define QAT_AST_EXPRESSIONS_UNION_INITIALISER_HPP

#include "../expression.hpp"

namespace qat::ast {

class UnionInitialiser : public Expression {
  friend class UnionInitialiser;

private:
  u32                 relative;
  String              typeName;
  String              subName;
  Maybe<Expression *> expression;

  IR::LocalValue *local = nullptr;
  String          irName;
  bool            isVar = false;

  // FIXME - Support template union types
public:
  UnionInitialiser(u32 relative, String name, String subName,
                   Maybe<Expression *> expression, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::unionInitialiser; }
};

} // namespace qat::ast

#endif