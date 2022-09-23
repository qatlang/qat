#ifndef QAT_AST_EXPRESSIONS_MIX_TYPE_INITIALISER_HPP
#define QAT_AST_EXPRESSIONS_MIX_TYPE_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class MixTypeInitialiser : public Expression {
  friend class LocalDeclaration;

private:
  QatType            *type;
  String              subName;
  Maybe<Expression *> expression;

  IR::LocalValue *local = nullptr;
  String          irName;
  bool            isVar = false;

public:
  MixTypeInitialiser(QatType *type, String subName,
                     Maybe<Expression *> expression,
                     utils::FileRange    fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType nodeType() const final { return NodeType::mixTypeInitialiser; }
};

} // namespace qat::ast

#endif