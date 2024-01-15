#include "core_type.hpp"
#include "../../ast/define_core_type.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../../show.hpp"
#include "../generics.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "./expanded_type.hpp"
#include "./qat_type.hpp"
#include "reference.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <utility>

namespace qat::IR {

CoreType::CoreType(QatModule* mod, Identifier _name, Vec<GenericParameter*> _generics, IR::OpaqueType* _opaqued,
                   Vec<Member*> _members, const VisibilityInfo& _visibility, llvm::LLVMContext& llctx,
                   Maybe<MetaInfo> _metaInfo)
    : ExpandedType(std::move(_name), _generics, mod, _visibility), EntityOverview("coreType", Json(), _name.range),
      opaquedType(_opaqued), members(std::move(_members)), metaInfo(_metaInfo) {
  SHOW("Generating LLVM Type for core type members")
  Maybe<String> foreignID;
  if (metaInfo) {
    foreignID = metaInfo->getForeignID();
  }
  Vec<llvm::Type*> subtypes;
  for (auto* mem : members) {
    subtypes.push_back(mem->type->getLLVMType());
  }
  SHOW("Opaqued type is: " << opaquedType)
  SHOW("All members' LLVM types obtained")
  llvmType    = opaquedType->getLLVMType();
  linkingName = opaquedType->getNameForLinking();
  llvm::cast<llvm::StructType>(llvmType)->setBody(subtypes, false);
  if (!isGeneric()) {
    mod->coreTypes.push_back(this);
  }
  if (opaquedType) {
    opaquedType->setSubType(this);
    ovInfo            = opaquedType->ovInfo;
    ovMentions        = opaquedType->ovMentions;
    ovBroughtMentions = opaquedType->ovBroughtMentions;
    ovRange           = opaquedType->ovRange;
  }
}

LinkNames CoreType::getLinkNames() const {
  Maybe<String> foreignID;
  if (metaInfo) {
    foreignID = metaInfo->getForeignID();
  }
  auto linkNames = parent->getLinkNames().newWith(LinkNameUnit(name.value, LinkUnitType::type), foreignID);
  if (isGeneric()) {
    Vec<LinkNames> genericlinkNames;
    for (auto* param : generics) {
      if (param->isTyped()) {
        genericlinkNames.push_back(
            LinkNames({LinkNameUnit(param->asTyped()->getType()->getNameForLinking(), LinkUnitType::genericTypeValue)},
                      None, nullptr));
      } else if (param->isPrerun()) {
        auto* preRes = param->asPrerun();
        genericlinkNames.push_back(
            LinkNames({LinkNameUnit(preRes->getType()->toPrerunGenericString(preRes->getExpression()).value(),
                                    LinkUnitType::genericPrerunValue)},
                      None, nullptr));
      }
    }
    linkNames.addUnit(LinkNameUnit("", LinkUnitType::genericList, None, genericlinkNames), None);
  }
  return linkNames;
}

CoreType::~CoreType() {
  for (auto* mem : members) {
    delete mem;
  }
  for (auto* gen : generics) {
    delete gen;
  }
}

void CoreType::updateOverview() {
  Vec<JsonValue> memJson;
  for (auto* mem : members) {
    memJson.push_back(mem->overviewToJson());
  }
  Vec<JsonValue> statMemJson;
  for (auto* statMem : staticMembers) {
    statMemJson.push_back(statMem->overviewToJson());
  }
  Vec<JsonValue> memFnJson;
  for (auto* mFn : memberFunctions) {
    memFnJson.push_back(mFn->overviewToJson());
  }
  ovInfo = Json()
               ._("typeID", getID())
               ._("fullName", getFullName())
               ._("moduleID", parent->getID())
               ._("members", memJson)
               ._("staticMembers", statMemJson)
               ._("memberFunctions", memFnJson)
               ._("hasDefaultConstructor", defaultConstructor != nullptr)
               ._("hasDestructor", destructor.has_value())
               ._("hasCopyConstructor", copyConstructor.has_value())
               ._("hasMoveConstructor", moveConstructor.has_value())
               ._("hasCopyAssignment", copyAssignment.has_value())
               ._("hasMoveAssignment", moveAssignment.has_value())
               ._("visibility", visibility);
}

u64 CoreType::getMemberCount() const { return members.size(); }

Vec<CoreType::Member*>& CoreType::getMembers() { return members; }

bool CoreType::hasMember(const String& member) const {
  for (auto* mem : members) {
    if (mem->name.value == member) {
      return true;
    }
  }
  return false;
}

CoreType::Member* CoreType::getMember(const String& member) const {
  for (auto* mem : members) {
    if (mem->name.value == member) {
      return mem;
    }
  }
  return nullptr;
}

CoreType::Member* CoreType::getMemberAt(u64 index) { return members.at(index); }

Maybe<usize> CoreType::getIndexOf(const String& member) const {
  Maybe<usize> result;
  for (usize i = 0; i < members.size(); i++) {
    if (members.at(i)->name.value == member) {
      result = i;
      break;
    }
  }
  return result;
}

String CoreType::getMemberNameAt(u64 index) const {
  return (index < members.size()) ? members.at(index)->name.value : "";
}

usize CoreType::getMemberIndex(String const& memName) const {
  for (usize i = 0; i < members.size(); i++) {
    if (members[i]->name.value == memName) {
      return i;
    }
  }
}

QatType* CoreType::getTypeOfMember(const String& member) const {
  Maybe<usize> result;
  for (usize i = 0; i < members.size(); i++) {
    if (members.at(i)->name.value == member) {
      result = i;
      break;
    }
  }
  if (result) {
    return members.at(result.value())->type;
  } else {
    return nullptr;
  }
}

bool CoreType::hasStatic(const String& _name) const {
  bool result = false;
  for (auto* stm : staticMembers) {
    if (stm->getName().value == _name) {
      return true;
    }
  }
  return result;
}

bool CoreType::isTypeSized() const { return !members.empty(); }

bool CoreType::isTriviallyCopyable() const {
  if (explicitTrivialCopy) {
    return true;
  } else if (hasCopyConstructor() || hasCopyAssignment()) {
    return false;
  } else {
    auto result = true;
    for (auto mem : members) {
      if (!mem->type->isTriviallyCopyable()) {
        result = false;
        break;
      }
    }
    return result;
  }
}

bool CoreType::isTriviallyMovable() const {
  if (explicitTrivialMove) {
    return true;
  } else if (hasMoveConstructor() || hasMoveAssignment()) {
    return false;
  } else {
    for (auto mem : members) {
      if (!mem->type->isTriviallyMovable()) {
        return false;
      }
    }
    return true;
  }
}

bool CoreType::isCopyConstructible() const {
  if (isTriviallyCopyable() || hasCopyConstructor()) {
    return true;
  } else {
    bool allMemsRes = true;
    for (auto* mem : members) {
      if (!mem->type->isCopyConstructible()) {
        allMemsRes = false;
        break;
      }
    }
    return allMemsRes;
  }
}

bool CoreType::isCopyAssignable() const {
  if (isTriviallyCopyable() || hasCopyAssignment()) {
    return true;
  } else {
    bool allMemsRes = true;
    for (auto* mem : members) {
      if (!mem->type->isCopyAssignable()) {
        allMemsRes = false;
        break;
      }
    }
    return allMemsRes;
  }
}

bool CoreType::isMoveConstructible() const {
  if (isTriviallyMovable() || hasMoveConstructor()) {
    return true;
  } else {
    bool allMemsRes = true;
    for (auto* mem : members) {
      if (!mem->type->isMoveConstructible()) {
        allMemsRes = false;
        break;
      }
    }
    return allMemsRes;
  }
}

bool CoreType::isMoveAssignable() const {
  if (isTriviallyMovable() || hasMoveAssignment()) {
    return true;
  } else {
    bool allMemsRes = true;
    for (auto* mem : members) {
      if (!mem->type->isMoveAssignable()) {
        allMemsRes = false;
        break;
      }
    }
    return allMemsRes;
  }
}

bool CoreType::isDestructible() const {
  if (isTriviallyMovable() || hasDestructor()) {
    return true;
  } else {
    bool allMemsRes = true;
    for (auto* mem : members) {
      if (!mem->type->isDestructible()) {
        allMemsRes = false;
        break;
      }
    }
    return allMemsRes;
  }
}

void CoreType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isCopyConstructible()) {
    if (isTriviallyCopyable()) {
      ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
    } else if (hasCopyConstructor()) {
      (void)getCopyConstructor()->call(ctx, {first->getLLVM(), second->getLLVM()}, None, ctx->getMod());
    } else {
      for (usize i = 0; i < members.size(); i++) {
        members.at(i)->type->copyConstructValue(
            ctx,
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), i),
                          IR::ReferenceType::get(true, members.at(i)->type, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), i),
                          IR::ReferenceType::get(false, members.at(i)->type, ctx), false, IR::Nature::temporary),
            fun);
      }
    }
  } else {
    ctx->Error("Could not copy construct an instance of type " + ctx->highlightError(toString()), None);
  }
}

void CoreType::copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isCopyAssignable()) {
    if (isTriviallyCopyable()) {
      ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
    } else if (hasCopyAssignment()) {
      (void)getCopyAssignment()->call(ctx, {first->getLLVM(), second->getLLVM()}, None, ctx->getMod());
    } else {
      for (usize i = 0; i < members.size(); i++) {
        members.at(i)->type->copyAssignValue(
            ctx,
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), i),
                          IR::ReferenceType::get(true, members.at(i)->type, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), i),
                          IR::ReferenceType::get(false, members.at(i)->type, ctx), false, IR::Nature::temporary),
            fun);
      }
    }
  } else {
    ctx->Error("Could not copy assign an instance of type " + ctx->highlightError(toString()), None);
  }
}
void CoreType::moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isMoveConstructible()) {
    if (isTriviallyMovable()) {
      ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
      ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), second->getLLVM());
    } else if (hasMoveConstructor()) {
      (void)getMoveConstructor()->call(ctx, {first->getLLVM(), second->getLLVM()}, None, ctx->getMod());
    } else {
      for (usize i = 0; i < members.size(); i++) {
        members.at(i)->type->moveConstructValue(
            ctx,
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), i),
                          IR::ReferenceType::get(true, members.at(i)->type, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), i),
                          IR::ReferenceType::get(false, members.at(i)->type, ctx), false, IR::Nature::temporary),
            fun);
      }
    }
  } else {
    ctx->Error("Could not move construct an instance of type " + ctx->highlightError(toString()), None);
  }
}
void CoreType::moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isMoveAssignable()) {
    if (isTriviallyMovable()) {
      ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
      ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), second->getLLVM());
    } else if (hasMoveAssignment()) {
      (void)getMoveAssignment()->call(ctx, {first->getLLVM(), second->getLLVM()}, None, ctx->getMod());
    } else {
      for (usize i = 0; i < members.size(); i++) {
        members.at(i)->type->moveAssignValue(
            ctx,
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), i),
                          IR::ReferenceType::get(true, members.at(i)->type, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), i),
                          IR::ReferenceType::get(false, members.at(i)->type, ctx), false, IR::Nature::temporary),
            fun);
      }
    }
  } else {
    ctx->Error("Could not move assign an instance of type " + ctx->highlightError(toString()), None);
  }
}
void CoreType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  if (isDestructible()) {
    if (hasDestructor()) {
      (void)getDestructor()->call(ctx, {instance->getLLVM()}, None, ctx->getMod());
    } else if (isTriviallyMovable()) {
      ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), instance->getLLVM());
    } else {
      for (usize i = 0; i < members.size(); i++) {
        members.at(i)->type->destroyValue(
            ctx,
            new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), instance->getLLVM(), i),
                          IR::ReferenceType::get(true, members.at(i)->type, ctx), false, IR::Nature::temporary),
            fun);
      }
    }
  } else {
    ctx->Error("Could not destroy an instance of type " + ctx->highlightError(toString()), None);
  }
}

void CoreType::addStaticMember(const Identifier& name, QatType* type, bool variability, Value* initial,
                               const VisibilityInfo& visibility, llvm::LLVMContext& llctx) {
  staticMembers.push_back(new StaticMember(this, name, type, variability, initial, visibility));
}

TypeKind CoreType::typeKind() const { return TypeKind::core; }

String CoreType::toString() const { return getFullName(); }

GenericCoreType::GenericCoreType(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                 Maybe<ast::PrerunExpression*> _constraint, ast::DefineCoreType* _defineCoreType,
                                 QatModule* _parent, const VisibilityInfo& _visibInfo)
    : EntityOverview("genericCoreType",
                     Json()
                         ._("name", _name.value)
                         ._("fullName", _parent->getFullNameWithChild(_name.value))
                         ._("visibility", _visibInfo)
                         ._("moduleID", _parent->getID()),
                     _name.range),
      name(std::move(_name)), generics(_generics), defineCoreType(_defineCoreType), parent(_parent),
      visibility(_visibInfo), constraint(_constraint) {
  parent->genericCoreTypes.push_back(this);
}

Identifier GenericCoreType::getName() const { return name; }

VisibilityInfo GenericCoreType::getVisibility() const { return visibility; }

bool GenericCoreType::allTypesHaveDefaults() const {
  for (auto* gen : generics) {
    if (!gen->hasDefault()) {
      return false;
    }
  }
  return true;
}

usize GenericCoreType::getTypeCount() const { return generics.size(); }

usize GenericCoreType::getVariantCount() const { return variants.size(); }

QatModule* GenericCoreType::getModule() const { return parent; }

ast::GenericAbstractType* GenericCoreType::getGenericAt(usize index) const { return generics.at(index); }

QatType* GenericCoreType::fillGenerics(Vec<GenericToFill*>& toFillTypes, IR::Context* ctx, FileRange range) {
  for (auto& oVar : opaqueVariants) {
    SHOW("Opaque variant: " << oVar.get()->getFullName())
    if (oVar.check(
            ctx, [&](const String& msg, const FileRange& rng) { ctx->Error(msg, rng); }, toFillTypes)) {
      return oVar.get();
    }
  }
  for (auto& var : variants) {
    SHOW("Core type variant: " << var.get()->getFullName())
    if (var.check(
            ctx, [&](const String& msg, const FileRange& rng) { ctx->Error(msg, rng); }, toFillTypes)) {
      return var.get();
    }
  }
  IR::fillGenerics(ctx, generics, toFillTypes, range);
  if (constraint.has_value()) {
    auto checkVal = constraint.value()->emit(ctx);
    if (checkVal->getType()->isBool()) {
      if (!llvm::cast<llvm::ConstantInt>(checkVal->getLLVMConstant())->getValue().getBoolValue()) {
        ctx->Error("The provided parameters for the generic struct type do not satisfy the constraints", range,
                   Pair<String, FileRange>{"The constraint can be found here", constraint.value()->fileRange});
      }
    } else {
      ctx->Error("The constraints for generic parameters should be of " + ctx->highlightError("bool") +
                     " type. Got an expression of " + ctx->highlightError(checkVal->getType()->toString()),
                 constraint.value()->fileRange);
    }
  }
  Vec<IR::GenericParameter*> genParams;
  for (auto genAb : generics) {
    genParams.push_back(genAb->toIRGenericType());
  }
  auto variantName = IR::Logic::getGenericVariantName(name.value, toFillTypes);
  for (auto varName : variantNames) {
    if (varName == variantName) {
      ctx->Error("Repeating variant name: " + variantName, range);
    }
  }
  variantNames.push_back(variantName);
  ctx->addActiveGeneric(
      IR::GenericEntityMarker{
          variantName,
          IR::GenericEntityType::coreType,
          range,
          0u,
          genParams,
      },
      true);
  defineCoreType->genericsToFill = toFillTypes;
  (void)defineCoreType->define(ctx);
  auto* cTy = defineCoreType->getCoreType();
  defineCoreType->setCoreType(cTy);
  (void)defineCoreType->emit(ctx);
  defineCoreType->unsetCoreType();
  for (auto* temp : generics) {
    temp->unset();
  }
  if (ctx->getActiveGeneric().warningCount > 0) {
    auto count = ctx->getActiveGeneric().warningCount;
    ctx->removeActiveGeneric();
    ctx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                     " generated while creating generic variant " + ctx->highlightWarning(variantName),
                 range);
  } else {
    ctx->removeActiveGeneric();
  }
  SHOW("Returning core type")
  return cTy;
}

} // namespace qat::IR