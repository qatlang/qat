#ifndef QAT_AST_PRERUN_MEMBER_ACCESS_HPP
#define QAT_AST_PRERUN_MEMBER_ACCESS_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunMemberAccess : public PrerunExpression {
  PrerunExpression* expr;
  Identifier        memberName;

public:
  PrerunMemberAccess(PrerunExpression* _expr, Identifier _memberName, FileRange _fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx);
  useit Json             toJson() const;
  useit String           toString() const;
  useit NodeType         nodeType() const { return NodeType::prerunMemberAccess; }
};

} // namespace qat::ast

#endif