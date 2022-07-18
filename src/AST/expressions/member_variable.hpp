#ifndef QAT_AST_EXPRESSIONS_MEMBER_VARIABLE_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_VARIABLE_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include <string>

namespace qat {
namespace AST {
class MemberVariable : public Expression {
  Expression *instance;

  std::string memberName;

  bool isPointerAccess;

public:
  MemberVariable(Expression *_instance, bool _isPointerAccess,
                 std::string _memberName, utils::FilePlacement _filePlacement)
      : instance(_instance), isPointerAccess(_isPointerAccess),
        memberName(_memberName), Expression(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::memberVariableExpression; }
};
} // namespace AST
} // namespace qat

#endif