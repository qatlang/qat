#ifndef QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat {
namespace AST {
class MemberIndexAccess : public Expression {
  Expression *instance;
  Expression *index;

public:
  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::memberIndexAccess; }
};
} // namespace AST
} // namespace qat

#endif