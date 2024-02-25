#ifndef QAT_AST_TYPE_DEFINITION_HPP
#define QAT_AST_TYPE_DEFINITION_HPP

#include "./node.hpp"
#include "expression.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class TypeDefinition : public IsEntity {
private:
  Identifier name;
  QatType*   subType;

  Maybe<PrerunExpression*> checker;
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
      : IsEntity(_fileRange), name(_name), subType(_subType), checker(_checker), constraint(_constraint),
        visibSpec(_visibSpec), generics(_generics) {}

  useit static inline TypeDefinition* create(Identifier _name, Maybe<PrerunExpression*> _checker,
                                             Vec<ast::GenericAbstractType*> _generics,
                                             Maybe<PrerunExpression*> _constraint, QatType* _subType,
                                             FileRange _fileRange, Maybe<VisibilitySpec> _visibSpec) {
    return std::construct_at(OwnNormal(TypeDefinition), _name, _checker, _generics, _constraint, _subType, _fileRange,
                             _visibSpec);
  }

  mutable Maybe<usize> typeSize;

  void setVariantName(const String& name) const;
  void unsetVariantName() const;
  void create_type(IR::QatModule* mod, IR::Context* ctx) const;

  void create_entity(IR::QatModule* parent, IR::Context* ctx) final;
  void update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) final;
  void do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) final;

  useit bool isGeneric() const;
  useit IR::DefinitionType* getDefinition() const;

  useit NodeType nodeType() const final { return NodeType::TYPE_DEFINITION; }
  useit Json     toJson() const final;
};

} // namespace qat::ast

#endif