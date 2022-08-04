#ifndef QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class MemberIndexAccess : public Expression {
  Expression *instance;
  Expression *index;

public:
  MemberIndexAccess(Expression *_instance, Expression *_index,
                    utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::memberIndexAccess;
  }
};

} // namespace qat::ast

#endif