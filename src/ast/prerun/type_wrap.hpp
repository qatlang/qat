#ifndef QAT_AST_PRERUN_TYPE_WRAPPING_HPP
#define QAT_AST_PRERUN_TYPE_WRAPPING_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class TypeWrap : public PrerunExpression {
  ast::QatType* theType;

public:
  TypeWrap(ast::QatType* theType, FileRange fileRange);

  IR::PrerunValue* emit(IR::Context* ctx);
  Json             toJson() const;
  String           toString() const;
  NodeType         nodeType() const { return NodeType::typeWrap; }
};

} // namespace qat::ast

#endif