#ifndef QAT_AST_PRERUN_TYPE_WRAPPING_HPP
#define QAT_AST_PRERUN_TYPE_WRAPPING_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class TypeWrap : public PrerunExpression {
  ast::QatType* theType;

public:
  TypeWrap(ast::QatType* _theType, FileRange _fileRange) : PrerunExpression(_fileRange), theType(_theType) {}

  useit static inline TypeWrap* create(ast::QatType* _theType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(TypeWrap), _theType, _fileRange);
  }

  IR::PrerunValue* emit(IR::Context* ctx);
  Json             toJson() const;
  String           toString() const;
  NodeType         nodeType() const { return NodeType::TYPE_WRAP; }
};

} // namespace qat::ast

#endif