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
  bool             isExpSelf = false;
  Identifier       memberName;
  Vec<Expression*> arguments;
  Maybe<bool>      callNature;

public:
  MemberFunctionCall(Expression* _instance, bool _isExpSelf, Identifier _memberName, Vec<Expression*> _arguments,
                     Maybe<bool> _variation, FileRange _fileRange)
      : Expression(std::move(_fileRange)), instance(_instance), isExpSelf(_isExpSelf),
        memberName(std::move(_memberName)), arguments(std::move(_arguments)), callNature(_variation) {}

  useit static inline MemberFunctionCall* create(Expression* _instance, bool _isExpSelf, Identifier _memberName,
                                                 Vec<Expression*> _arguments, Maybe<bool> _variation,
                                                 FileRange _fileRange) {
    return std::construct_at(OwnNormal(MemberFunctionCall), _instance, _isExpSelf, _memberName, _arguments, _variation,
                             _fileRange);
  }

  IR::Value* emit(IR::Context* ctx) override;

  useit Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::MEMBER_FUNCTION_CALL; }
};

} // namespace qat::ast

#endif