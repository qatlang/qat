#ifndef QAT_AST_TYPE_DEFINITION_HPP
#define QAT_AST_TYPE_DEFINITION_HPP

#include "./node.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class TypeDefinition : public Node {
private:
  Identifier            name;
  QatType*              subType;
  utils::VisibilityKind visibKind;

public:
  TypeDefinition(Identifier _name, QatType* _subType, FileRange _fileRange, utils::VisibilityKind _visibKind);

  void  defineType(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* _) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::typeDefinition; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif