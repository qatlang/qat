#ifndef QAT_AST_CONSTANTS_TYPE_CHECKER_HPP
#define QAT_AST_CONSTANTS_TYPE_CHECKER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class TypeChecker : public ConstantExpression {
private:
  String             name;
  Vec<ast::QatType*> args;

public:
  TypeChecker(String name, Vec<ast::QatType*> args, FileRange fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) final;
  useit Json               toJson() const final;
  useit NodeType           nodeType() const final { return NodeType::typeChecker; }
};

} // namespace qat::ast

#endif