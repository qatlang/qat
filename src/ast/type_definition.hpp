#ifndef QAT_AST_TYPE_DEFINITION_HPP
#define QAT_AST_TYPE_DEFINITION_HPP

#include "./node.hpp"
#include "expression.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class TypeDefinition : public Node {
private:
  Identifier               name;
  Maybe<PrerunExpression*> checker;
  QatType*                 subType;
  Maybe<PrerunExpression*> constraint;
  Maybe<VisibilitySpec>    visibSpec;

  Vec<ast::GenericAbstractType*>     generics;
  mutable Maybe<String>              variantName;
  mutable IR::DefinitionType*        typeDefinition        = nullptr;
  mutable IR::GenericDefinitionType* genericTypeDefinition = nullptr;
  mutable Maybe<bool>                checkResult;

public:
  TypeDefinition(Identifier _name, Maybe<PrerunExpression*> _checker, Vec<ast::GenericAbstractType*> _generics,
                 Maybe<PrerunExpression*> _constraint, QatType* _subType, FileRange _fileRange,
                 Maybe<VisibilitySpec> _visibSpec)
      : Node(_fileRange), name(_name), checker(_checker), subType(_subType), constraint(_constraint),
        visibSpec(_visibSpec), generics(_generics) {}

  useit static inline TypeDefinition* create(Identifier _name, Maybe<PrerunExpression*> _checker,
                                             Vec<ast::GenericAbstractType*> _generics,
                                             Maybe<PrerunExpression*> _constraint, QatType* _subType,
                                             FileRange _fileRange, Maybe<VisibilitySpec> _visibSpec) {
    return std::construct_at(OwnNormal(TypeDefinition), _name, _checker, _generics, _constraint, _subType, _fileRange,
                             _visibSpec);
  }

  void setVariantName(const String& name) const;
  void unsetVariantName() const;
  void createType(IR::Context* ctx) const;
  void defineType(IR::Context* ctx) final;
  void define(IR::Context* ctx) final;

  useit bool isGeneric() const;
  useit IR::DefinitionType* getDefinition() const;
  useit IR::Value* emit(IR::Context* _) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::TYPE_DEFINITION; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif