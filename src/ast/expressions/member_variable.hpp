#ifndef QAT_AST_EXPRESSIONS_MEMBER_VARIABLE_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_VARIABLE_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include <string>

namespace qat::ast {
class MemberVariable : public Expression {
  Expression *instance;

  String memberName;

  bool isPointerAccess;

public:
  MemberVariable(Expression *_instance, bool _isPointerAccess,
                 String _memberName, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::memberVariableExpression;
  }
};

} // namespace qat::ast

#endif