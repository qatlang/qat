#ifndef QAT_AST_EXPRESSIONS_MEMBER_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_ACCESS_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include <string>

namespace qat::ast {
class MemberAccess : public Expression {
  Expression *instance;
  String      name;
  bool        isPointer;

public:
  MemberAccess(Expression *_instance, bool _isPointer, String _name,
               utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) override;
  useit nuo::Json toJson() const override;
  useit NodeType  nodeType() const override { return NodeType::memberAccess; }
};

} // namespace qat::ast

#endif