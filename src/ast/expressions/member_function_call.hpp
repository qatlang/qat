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
  Expression*      instance;
  String           memberName;
  Vec<Expression*> arguments;
  bool             variation;

public:
  MemberFunctionCall(Expression* _instance, String _memberName, Vec<Expression*> _arguments, bool _variation,
                     FileRange _fileRange)
      : Expression(std::move(_fileRange)), instance(_instance), memberName(std::move(_memberName)),
        arguments(std::move(_arguments)), variation(_variation) {}

  IR::Value* emit(IR::Context* ctx) override;

  useit Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::memberFunctionCall; }
};

} // namespace qat::ast

#endif