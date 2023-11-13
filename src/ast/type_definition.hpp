#ifndef QAT_AST_TYPE_DEFINITION_HPP
#define QAT_AST_TYPE_DEFINITION_HPP

#include "./node.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class TypeDefinition : public Node {
private:
  Identifier            name;
  QatType*              subType;
  Maybe<VisibilitySpec> visibSpec;

  Vec<ast::GenericAbstractType*>     generics;
  mutable Maybe<String>              variantName;
  mutable IR::DefinitionType*        typeDefinition        = nullptr;
  mutable IR::GenericDefinitionType* genericTypeDefinition = nullptr;

public:
  TypeDefinition(Identifier _name, Vec<ast::GenericAbstractType*> _generics, QatType* _subType, FileRange _fileRange,
                 Maybe<VisibilitySpec> _visibSpec);

  void setVariantName(const String& name) const;
  void unsetVariantName() const;
  void createType(IR::Context* ctx) const;
  void defineType(IR::Context* ctx) final;
  void define(IR::Context* ctx) final;

  useit bool isGeneric() const;
  useit IR::DefinitionType* getDefinition() const;
  useit IR::Value* emit(IR::Context* _) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::typeDefinition; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif