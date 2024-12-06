#ifndef QAT_AST_TYPE_DEFINITION_HPP
#define QAT_AST_TYPE_DEFINITION_HPP

#include "./node.hpp"
#include "expression.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class TypeDefinition : public IsEntity {
private:
  Identifier name;
  Type*      subType;

  Maybe<PrerunExpression*> checker;
  Maybe<PrerunExpression*> constraint;
  Maybe<VisibilitySpec>    visibSpec;

  Vec<ast::GenericAbstractType*>     generics;
  mutable Maybe<String>              variantName;
  mutable ir::DefinitionType*        typeDefinition        = nullptr;
  mutable ir::GenericDefinitionType* genericTypeDefinition = nullptr;
  mutable Maybe<bool>                checkResult;

public:
  TypeDefinition(Identifier _name, Maybe<PrerunExpression*> _checker, Vec<ast::GenericAbstractType*> _generics,
                 Maybe<PrerunExpression*> _constraint, Type* _subType, FileRange _fileRange,
                 Maybe<VisibilitySpec> _visibSpec)
      : IsEntity(_fileRange), name(_name), subType(_subType), checker(_checker), constraint(_constraint),
        visibSpec(_visibSpec), generics(_generics) {}

  useit static TypeDefinition* create(Identifier _name, Maybe<PrerunExpression*> _checker,
                                      Vec<ast::GenericAbstractType*> _generics, Maybe<PrerunExpression*> _constraint,
                                      Type* _subType, FileRange _fileRange, Maybe<VisibilitySpec> _visibSpec) {
    return std::construct_at(OwnNormal(TypeDefinition), _name, _checker, _generics, _constraint, _subType, _fileRange,
                             _visibSpec);
  }

  mutable Maybe<usize> typeSize;

  void set_variant_name(const String& name) const;
  void unset_variant_name() const;
  void create_type(ir::Mod* mod, ir::Ctx* irCtx) const;

  void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
  void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;
  void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

  useit bool isGeneric() const;
  useit ir::DefinitionType* getDefinition() const;

  useit NodeType nodeType() const final { return NodeType::TYPE_DEFINITION; }
  useit Json     to_json() const final;
};

} // namespace qat::ast

#endif
