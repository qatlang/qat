#ifndef QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class IndexAccess : public Expression {
  Expression* instance;
  Expression* index;

public:
  IndexAccess(Expression* _instance, Expression* _index, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::memberIndexAccess; }
};

} // namespace qat::ast

#endif