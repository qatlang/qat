#include "core_type.hpp"
#include "../../ast/define_core_type.hpp"
#include "../../ast/types/templated.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "definition.hpp"
#include "function.hpp"
#include "reference.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <utility>

namespace qat::IR {

CoreType::CoreType(QatModule* mod, Identifier _name, Vec<Member*> _members, const utils::VisibilityInfo& _visibility,
                   llvm::LLVMContext& ctx)
    : EntityOverview("coreType", Json(), _name.range), name(std::move(_name)), parent(mod),
      members(std::move(_members)), visibility(_visibility) {
  SHOW("Generating LLVM Type for core type members")
  Vec<llvm::Type*> subtypes;
  for (auto* mem : members) {
    subtypes.push_back(mem->type->getLLVMType());
  }
  SHOW("All members' LLVM types obtained")
  llvmType = llvm::StructType::create(ctx, subtypes, mod->getFullNameWithChild(name.value), false);
  if (mod) {
    mod->coreTypes.push_back(this);
  }
}

CoreType::~CoreType() {
  for (auto* mem : members) {
    delete mem;
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
               ._("hasDestructor", destructor != nullptr)
               ._("hasCopyConstructor", copyConstructor.has_value())
               ._("hasMoveConstructor", moveConstructor.has_value())
               ._("hasCopyAssignment", copyAssignment.has_value())
               ._("hasMoveAssignment", moveAssignment.has_value())
               ._("visibility", visibility);
}

String CoreType::getFullName() const { return parent->getFullNameWithChild(name.value); }

Identifier CoreType::getName() const { return name; }

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

bool CoreType::hasMemberFunction(const String& fnName) const {
  for (auto* memberFunction : memberFunctions) {
    SHOW("Found member function: " << memberFunction->getName().value)
    if (memberFunction->getName().value == fnName) {
      return true;
    }
  }
  return false;
}

MemberFunction* CoreType::getMemberFunction(const String& fnName) const {
  for (auto* memberFunction : memberFunctions) {
    if (memberFunction->getName().value == fnName) {
      return memberFunction;
    }
  }
  return nullptr;
}

bool CoreType::hasStaticFunction(const String& fnName) const {
  for (const auto& fun : staticFunctions) {
    if (fun->getName().value == fnName) {
      return true;
    }
  }
  return false;
}

MemberFunction* CoreType::getStaticFunction(const String& fnName) const {
  for (auto* staticFunction : staticFunctions) {
    if (staticFunction->getName().value == fnName) {
      return staticFunction;
    }
  }
  return nullptr;
}

void CoreType::addStaticMember(const Identifier& name, QatType* type, bool variability, Value* initial,
                               const utils::VisibilityInfo& visibility, llvm::LLVMContext& ctx) {
  staticMembers.push_back(new StaticMember(this, name, type, variability, initial, visibility));
}

bool CoreType::hasBinaryOperator(const String& opr, IR::QatType* type) const {
  for (auto* bin : binaryOperators) {
    if (utils::splitString(bin->getName().value, "'")[1] == opr) {
      auto* binArgTy = bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(type) || (binArgTy->isReference() && binArgTy->asReference()->getSubType()->isSame(type))) {
        return true;
      }
    }
  }
  return false;
}

MemberFunction* CoreType::getBinaryOperator(const String& opr, IR::QatType* type) const {
  for (auto* bin : binaryOperators) {
    if (utils::splitString(bin->getName().value, "'")[1] == opr) {
      auto* binArgTy = bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(type) || (binArgTy->isReference() && binArgTy->asReference()->getSubType()->isSame(type))) {
        return bin;
      }
    }
  }
  return nullptr;
}

bool CoreType::hasUnaryOperator(const String& opr) const {
  for (auto* unr : unaryOperators) {
    if (utils::splitString(unr->getName().value, "'")[1] == opr) {
      return true;
    }
  }
  return false;
}

MemberFunction* CoreType::getUnaryOperator(const String& opr) const {
  for (auto* unr : unaryOperators) {
    if (utils::splitString(unr->getName().value, "'")[1] == opr) {
      return unr;
    }
  }
  return nullptr;
}

u64 CoreType::getOperatorVariantIndex(const String& opr) const {
  u64 index = 0;
  for (auto* bin : binaryOperators) {
    if (utils::splitString(bin->getName().value, "'")[1] == opr) {
      index++;
    }
  }
  return index;
}

bool CoreType::hasDefaultConstructor() const { return defaultConstructor != nullptr; }

MemberFunction* CoreType::getDefaultConstructor() const { return defaultConstructor; }

bool CoreType::hasAnyConstructor() const { return (!constructors.empty()) || (defaultConstructor != nullptr); }

bool CoreType::hasAnyFromConvertor() const { return !fromConvertors.empty(); }

bool CoreType::hasConstructorWithTypes(Vec<IR::QatType*> types) const {
  for (auto* con : constructors) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto* argType = argTys.at(i)->getType();
        if (!argType->isSame(types.at(i - 1)) && !argType->isCompatible(types.at(i - 1)) &&
            (!(argType->isReference() && (argType->asReference()->getSubType()->isSame(types.at(i - 1)) ||
                                          argType->asReference()->getSubType()->isCompatible(types.at(i - 1))))) &&
            (!(types.at(i - 1)->isReference() &&
               (types.at(i - 1)->asReference()->getSubType()->isSame(argType) ||
                types.at(i - 1)->asReference()->getSubType()->isCompatible(argType))))) {
          result = false;
          break;
        }
      }
      if (result) {
        return true;
      }
    } else {
      continue;
    }
  }
  return false;
}

MemberFunction* CoreType::getConstructorWithTypes(Vec<IR::QatType*> types) const {
  for (auto* con : constructors) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto* argType = argTys.at(i)->getType();
        if (!argType->isSame(types.at(i - 1)) && !argType->isCompatible(types.at(i - 1)) &&
            (!(argType->isReference() && (argType->asReference()->getSubType()->isSame(types.at(i - 1)) ||
                                          argType->asReference()->getSubType()->isCompatible(types.at(i - 1))))) &&
            (!(types.at(i - 1)->isReference() && types.at(i - 1)->asReference()->getSubType()->isSame(argType)))) {
          result = false;
          break;
        }
      }
      if (result) {
        return con;
      }
    } else {
      continue;
    }
  }
  return nullptr;
}

bool CoreType::hasFromConvertor(IR::QatType* typ) const {
  for (auto* fconv : fromConvertors) {
    auto* argTy = fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(typ) || argTy->isCompatible(typ) ||
        (argTy->isReference() &&
         (argTy->asReference()->getSubType()->isSame(typ) || argTy->asReference()->getSubType()->isCompatible(typ))) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(argTy))) {
      return true;
    }
  }
  return false;
}

MemberFunction* CoreType::getFromConvertor(IR::QatType* typ) const {
  for (auto* fconv : fromConvertors) {
    auto* argTy = fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(typ) || argTy->isCompatible(typ) ||
        (argTy->isReference() &&
         (argTy->asReference()->getSubType()->isSame(typ) || argTy->asReference()->getSubType()->isCompatible(typ))) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(argTy))) {
      return fconv;
    }
  }
  return nullptr;
}

bool CoreType::hasToConvertor(IR::QatType* typ) const {
  for (auto* fconv : fromConvertors) {
    auto* retTy = fconv->getType()->asFunction()->getReturnType();
    if (retTy->isSame(typ) || (retTy->isReference() && retTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(retTy))) {
      return true;
    }
  }
  return false;
}

MemberFunction* CoreType::getToConvertor(IR::QatType* typ) const {
  for (auto* tconv : fromConvertors) {
    auto* retTy = tconv->getType()->asFunction()->getReturnType();
    if (retTy->isSame(typ) || (retTy->isReference() && retTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(retTy))) {
      return tconv;
    }
  }
  return nullptr;
}

bool CoreType::hasCopyConstructor() const { return copyConstructor.has_value(); }

MemberFunction* CoreType::getCopyConstructor() const { return copyConstructor.value_or(nullptr); }

bool CoreType::hasMoveConstructor() const { return moveConstructor.has_value(); }

MemberFunction* CoreType::getMoveConstructor() const { return moveConstructor.value_or(nullptr); }

bool CoreType::hasCopyAssignment() const { return copyAssignment.has_value(); }

MemberFunction* CoreType::getCopyAssignment() const { return copyAssignment.value_or(nullptr); }

bool CoreType::hasMoveAssignment() const { return moveAssignment.has_value(); }

MemberFunction* CoreType::getMoveAssignment() const { return moveAssignment.value_or(nullptr); }

bool CoreType::isTriviallyCopyable() const {
  return (constructors.empty() && fromConvertors.empty() && !hasDestructor() && !hasCopyConstructor() &&
          !hasMoveConstructor());
}

bool CoreType::hasCopy() const { return hasCopyConstructor() && hasCopyAssignment(); }

bool CoreType::hasMove() const { return hasMoveConstructor() && hasMoveAssignment(); }

bool CoreType::hasDestructor() const { return destructor != nullptr; }

IR::MemberFunction* CoreType::getDestructor() const { return destructor; }

utils::VisibilityInfo CoreType::getVisibility() const { return visibility; }

QatModule* CoreType::getParent() { return parent; }

TypeKind CoreType::typeKind() const { return TypeKind::core; }

String CoreType::toString() const { return getFullName(); }

Json CoreType::toJson() const { return Json()._("id", getID())._("name", name); }

bool CoreType::isCopyExplicit() const { return explicitCopy; }

bool CoreType::isMoveExplicit() const { return explicitMove; }

void CoreType::setExplicitCopy() { explicitCopy = true; }

void CoreType::setExplicitMove() { explicitMove = true; }

TemplateCoreType::TemplateCoreType(Identifier _name, Vec<ast::TemplatedType*> _templates,
                                   ast::DefineCoreType* _defineCoreType, QatModule* _parent,
                                   const utils::VisibilityInfo& _visibInfo)
    : EntityOverview("genericCoreType",
                     Json()
                         ._("name", _name.value)
                         ._("fullName", _parent->getFullNameWithChild(_name.value))
                         ._("visibility", _visibInfo)
                         ._("moduleID", _parent->getID()),
                     _name.range),
      name(std::move(_name)), templates(std::move(_templates)), defineCoreType(_defineCoreType), parent(_parent),
      visibility(_visibInfo) {
  SHOW("Adding template core type to parent module")
  parent->templateCoreTypes.push_back(this);
}

Identifier TemplateCoreType::getName() const { return name; }

utils::VisibilityInfo TemplateCoreType::getVisibility() const { return visibility; }

usize TemplateCoreType::getTypeCount() const { return templates.size(); }

usize TemplateCoreType::getVariantCount() const { return variants.size(); }

QatModule* TemplateCoreType::getModule() const { return parent; }

CoreType* TemplateCoreType::fillTemplates(Vec<QatType*>& types, IR::Context* ctx, FileRange range) {
  for (auto var : variants) {
    if (var.check(types)) {
      return var.get();
    }
  }
  for (usize i = 0; i < templates.size(); i++) {
    templates.at(i)->setType(types.at(i));
  }
  auto variantName = IR::Logic::getTemplateVariantName(name.value, types);
  defineCoreType->setVariantName(variantName);
  auto prevTemp       = ctx->activeTemplate;
  ctx->activeTemplate = IR::TemplateEntityMarker{variantName, IR::TemplateEntityType::coreType, range};
  (void)defineCoreType->define(ctx);
  (void)defineCoreType->emit(ctx);
  auto* cTy = (IR::CoreType*)defineCoreType->getCoreType();
  variants.push_back(TemplateVariant<CoreType>(cTy, types));
  for (auto* temp : templates) {
    temp->unsetType();
  }
  defineCoreType->unsetVariantName();
  if (ctx->activeTemplate->warningCount > 0) {
    auto count          = ctx->activeTemplate->warningCount;
    ctx->activeTemplate = None;
    ctx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                     " generated while creating template variant " + ctx->highlightWarning(variantName),
                 range);
  }
  ctx->activeTemplate = prevTemp;
  SHOW("Created variant for template core type: " << cTy->getFullName())
  return cTy;
}

} // namespace qat::IR