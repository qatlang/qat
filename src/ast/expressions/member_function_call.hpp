#ifndef QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include <string>
#include <vector>

namespace qat::ast {

class MemberFunctionCall : public Expression {
private:
  Expression *instance;
  std::string memberName;
  std::vector<Expression *> arguments;
  bool variation;

public:
  MemberFunctionCall(Expression *_instance, std::string _memberName,
                     std::vector<Expression *> _arguments, bool _variation,
                     utils::FileRange _fileRange)
      : instance(_instance), memberName(_memberName), arguments(_arguments),
        variation(_variation), Expression(_fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::memberFunctionCall; }
};

} // namespace qat::ast

#endif