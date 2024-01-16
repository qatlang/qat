#ifndef QAT_AST_EXPRESSIONS_MEMBER_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_ACCESS_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class MemberAccess : public Expression {
  Expression* instance;
  bool        isExpSelf = false;
  bool        isPointerAccess;
  Maybe<bool> isVariationAccess;
  Identifier  name;

public:
  MemberAccess(Expression* _instance, bool _isExpSelf, bool _isPointerAccess, Maybe<bool> _isVariationAccess,
               Identifier _name, FileRange _fileRange)
      : Expression(std::move(_fileRange)), instance(_instance), isExpSelf(_isExpSelf),
        isPointerAccess(_isPointerAccess), isVariationAccess(_isVariationAccess), name(std::move(_name)) {}

  useit static inline MemberAccess* create(Expression* _instance, bool isExpSelf, bool _isPointerAccess,
                                           Maybe<bool> _isVariationAccess, Identifier _name, FileRange _fileRange) {
    return std::construct_at(OwnNormal(MemberAccess), _instance, isExpSelf, _isPointerAccess, _isVariationAccess, _name,
                             _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::MEMBER_ACCESS; }
};

} // namespace qat::ast

#endif