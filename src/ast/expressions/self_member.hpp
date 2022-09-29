#ifndef QAT_AST_EXPRESSIONS_SELF_MEMBER_HPP
#define QAT_AST_EXPRESSIONS_SELF_MEMBER_HPP

#include "../expression.hpp"

namespace qat::ast {

class SelfMember : public Expression {
private:
  String name;

public:
  SelfMember(String name, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::selfMember; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif