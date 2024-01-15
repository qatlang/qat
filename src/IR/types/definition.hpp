#ifndef QAT_IR_TYPES_DEFINITION_HPP
#define QAT_IR_TYPES_DEFINITION_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/helpers.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../generic_variant.hpp"
#include "./expanded_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ast {
class GenericAbstractType;
class TypeDefinition;
} // namespace qat::ast

namespace qat::IR {

class QatModule;

class DefinitionType : public ExpandedType, public EntityOverview {
private:
  QatType* subType;

public:
  DefinitionType(Identifier _name, QatType* _actualType, Vec<GenericParameter*> _generics, QatModule* mod,
                 const VisibilityInfo& _visibInfo);

  void setSubType(QatType* _subType);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit QatModule* getParent();
  useit QatType*   getSubType();
  useit TypeKind   typeKind() const final;
  useit LinkNames  getLinkNames() const final;
  useit String     toString() const final;
  useit bool       isExpanded() const final;
  useit bool       isTypeSized() const final;

  useit bool isTriviallyCopyable() const final;
  useit bool isTriviallyMovable() const final;
  useit bool isCopyConstructible() const final;
  useit bool isCopyAssignable() const final;
  useit bool isMoveConstructible() const final;
  useit bool isMoveAssignable() const final;
  useit bool isDestructible() const final;

  void copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) final;
  void updateOverview() final;

  useit bool canBePrerun() const final { return subType->canBePrerun(); }
  useit bool canBePrerunGeneric() const final { return subType->canBePrerunGeneric(); }
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* constant) const final;
  useit Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;

  useit VisibilityInfo getVisibility() const;
};

class GenericDefinitionType : public Uniq, public EntityOverview {
private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> generics;
  ast::TypeDefinition*           defineTypeDef;
  QatModule*                     parent;
  VisibilityInfo                 visibility;

  Maybe<ast::PrerunExpression*> constraint;

  mutable Vec<GenericVariant<DefinitionType>> variants;

public:
  GenericDefinitionType(Identifier name, Vec<ast::GenericAbstractType*> generics,
                        Maybe<ast::PrerunExpression*> constraint, ast::TypeDefinition* defineCoreType,
                        QatModule* parent, const VisibilityInfo& visibInfo);

  ~GenericDefinitionType() = default;

  useit Identifier      getName() const;
  useit usize           getTypeCount() const;
  useit bool            allTypesHaveDefaults() const;
  useit usize           getVariantCount() const;
  useit QatModule*      getModule() const;
  useit DefinitionType* fillGenerics(Vec<IR::GenericToFill*>& types, IR::Context* ctx, FileRange range);

  useit ast::GenericAbstractType* getGenericAt(usize index) const;
  useit VisibilityInfo            getVisibility() const;
};

} // namespace qat::IR

#endif