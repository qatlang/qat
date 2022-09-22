#ifndef QAT_AST_EXPRESSIONS_UNION_INITIALISER_HPP
#define QAT_AST_EXPRESSIONS_UNION_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class UnionInitialiser : public Expression {
  friend class LocalDeclaration;

private:
  QatType            *type;
  String              subName;
  Maybe<Expression *> expression;

  IR::LocalValue *local = nullptr;
  String          irName;
  bool            isVar = false;

public:
  UnionInitialiser(QatType *type, String subName,
                   Maybe<Expression *> expression, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::unionInitialiser; }
};

} // namespace qat::ast

#endif