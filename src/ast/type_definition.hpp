#ifndef QAT_AST_TYPEDEF_HPP
#define QAT_AST_TYPEDEF_HPP

#include "./expression.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class TypeDefinition : public Expression {
private:
  String                name;
  QatType              *subType;
  utils::VisibilityKind visibKind;

public:
  TypeDefinition(String _name, QatType *_subType, utils::FileRange _fileRange,
                 utils::VisibilityKind _visibKind);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::typeDefinition; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif