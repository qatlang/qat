#ifndef QAT_AST_SENTENCES_NEW_ALIAS_HPP
#define QAT_AST_SENTENCES_NEW_ALIAS_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class NewAlias : public Sentence {
private:
  String      name;
  Expression *exp;

public:
  NewAlias(String name, Expression *exp, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::newAlias; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif