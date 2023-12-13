#include "./definition.hpp"
#include "../../ast/type_definition.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

DefinitionType::DefinitionType(Identifier _name, QatType* _subType, Vec<GenericParameter*> _generics, QatModule* _mod,
                               const VisibilityInfo& _visibInfo)
    : ExpandedType(_name, _generics, _mod, _visibInfo), EntityOverview("typeDefinition", Json(), _name.range),
      subType(_subType) {
  setSubType(subType);
  linkingName = subType->getNameForLinking();
  parent->typeDefs.push_back(this);
}

LinkNames DefinitionType::getLinkNames() const {
  return LinkNames({LinkNameUnit(linkingName, LinkUnitType::typeName)}, None, nullptr);
}

void DefinitionType::setSubType(QatType* _subType) {
  subType = _subType;
  if (subType) {
    llvmType = subType->getLLVMType();
  }
}

VisibilityInfo DefinitionType::getVisibility() const { return visibility; }

bool DefinitionType::isExpanded() const { return subType->isExpanded(); }

bool DefinitionType::isCopyConstructible() const { return subType->isCopyConstructible(); }

bool DefinitionType::isCopyAssignable() const { return subType->isCopyAssignable(); }

bool DefinitionType::isMoveConstructible() const { return subType->isMoveConstructible(); }

bool DefinitionType::isMoveAssignable() const { return subType->isMoveAssignable(); }

bool DefinitionType::isDestructible() const { return subType->isDestructible(); }

void DefinitionType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  subType->copyConstructValue(ctx, first, second, fun);
}

void DefinitionType::copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  subType->copyAssignValue(ctx, first, second, fun);
}

void DefinitionType::moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  subType->moveConstructValue(ctx, first, second, fun);
}

void DefinitionType::moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  subType->moveAssignValue(ctx, first, second, fun);
}

void DefinitionType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  subType->destroyValue(ctx, instance, fun);
}

Identifier DefinitionType::getName() const { return name; }

String DefinitionType::getFullName() const { return parent ? parent->getFullNameWithChild(name.value) : name.value; }

QatModule* DefinitionType::getParent() { return parent; }

QatType* DefinitionType::getSubType() { return subType; }

bool DefinitionType::isTypeSized() const { return subType->isTypeSized(); }

bool DefinitionType::isTriviallyCopyable() const { return subType->isTriviallyCopyable(); }

bool DefinitionType::isTriviallyMovable() const { return subType->isTriviallyMovable(); }

void DefinitionType::updateOverview() {
  Vec<JsonValue> genJson;
  for (auto* gen : generics) {
    genJson.push_back(gen->toJson());
  }
  ovInfo._("fullName", getFullName())
      ._("typeID", getID())
      ._("subTypeID", subType->getID())
      ._("visibility", visibility)
      ._("hasGenerics", !generics.empty())
      ._("generics", genJson)
      ._("moduleID", parent->getID());
}

bool DefinitionType::canBePrerunGeneric() const { return subType->canBePrerunGeneric(); }

Maybe<String> DefinitionType::toPrerunGenericString(IR::PrerunValue* constant) const {
  if (subType->canBePrerunGeneric()) {
    return subType->toPrerunGenericString(constant);
  } else {
    return None;
  }
}

Maybe<bool> DefinitionType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (subType->canBePrerunGeneric()) {
    return subType->equalityOf(first, second);
  } else {
    return None;
  }
}

TypeKind DefinitionType::typeKind() const { return TypeKind::definition; }

String DefinitionType::toString() const { return getFullName(); }

GenericDefinitionType::GenericDefinitionType(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                             ast::TypeDefinition* _defineTypeDef, QatModule* _parent,
                                             const VisibilityInfo& _visibInfo)
    : EntityOverview("genericTypeDefinition",
                     Json()
                         ._("name", _name.value)
                         ._("fullName", _parent->getFullNameWithChild(_name.value))
                         ._("visibility", _visibInfo)
                         ._("moduleID", _parent->getID()),
                     _name.range),
      name(_name), generics(_generics), defineTypeDef(_defineTypeDef), parent(_parent), visibility(_visibInfo) {
  parent->genericTypeDefinitions.push_back(this);
}

Identifier GenericDefinitionType::getName() const { return name; }

VisibilityInfo GenericDefinitionType::getVisibility() const { return visibility; }

bool GenericDefinitionType::allTypesHaveDefaults() const {
  for (auto* gen : generics) {
    if (!gen->hasDefault()) {
      return false;
    }
  }
  return true;
}

usize GenericDefinitionType::getTypeCount() const { return generics.size(); }

usize GenericDefinitionType::getVariantCount() const { return variants.size(); }

QatModule* GenericDefinitionType::getModule() const { return parent; }

ast::GenericAbstractType* GenericDefinitionType::getGenericAt(usize index) const { return generics.at(index); }

DefinitionType* GenericDefinitionType::fillGenerics(Vec<GenericToFill*>& types, IR::Context* ctx, FileRange range) {
  for (auto var : variants) {
    if (var.check([&](const String& msg, const FileRange& rng) { ctx->Error(msg, rng); }, types)) {
      return var.get();
    }
  }
  IR::fillGenerics(ctx, generics, types, range);
  auto variantName = IR::Logic::getGenericVariantName(name.value, types);
  defineTypeDef->setVariantName(variantName);
  ctx->addActiveGeneric(IR::GenericEntityMarker{variantName, IR::GenericEntityType::typeDefinition, range}, true);
  (void)defineTypeDef->define(ctx);
  (void)defineTypeDef->emit(ctx);
  auto* dTy = defineTypeDef->getDefinition();
  variants.push_back(GenericVariant<DefinitionType>(dTy, types));
  for (auto* temp : generics) {
    temp->unset();
  }
  defineTypeDef->unsetVariantName();
  if (ctx->getActiveGeneric().warningCount > 0) {
    auto count = ctx->getActiveGeneric().warningCount;
    ctx->removeActiveGeneric();
    ctx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                     " generated while creating generic variant " + ctx->highlightWarning(variantName),
                 range);
  } else {
    ctx->removeActiveGeneric();
  }
  return dTy;
}

} // namespace qat::IR