#include "core_type.hpp"
#include "../../ast/define_core_type.hpp"
#include "../../ast/types/const_generic.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../../ast/types/typed_generic.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"
#include "../generics.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "./expanded_type.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <utility>

namespace qat::IR {

CoreType::CoreType(QatModule* mod, Identifier _name, Vec<GenericParameter*> _generics, Vec<Member*> _members,
                   const VisibilityInfo& _visibility, llvm::LLVMContext& llctx)
    : ExpandedType(std::move(_name), _generics, mod, _visibility), EntityOverview("coreType", Json(), _name.range),
      members(std::move(_members)) {
  SHOW("Generating LLVM Type for core type members")
  Vec<llvm::Type*> subtypes;
  for (auto* mem : members) {
    subtypes.push_back(mem->type->getLLVMType());
  }
  SHOW("All members' LLVM types obtained")
  llvmType = llvm::StructType::create(llctx, subtypes, mod->getFullNameWithChild(name.value), false);
  mod->coreTypes.push_back(this);
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
               ._("hasDestructor", destructor != nullptr)
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

void CoreType::addStaticMember(const Identifier& name, QatType* type, bool variability, Value* initial,
                               const VisibilityInfo& visibility, llvm::LLVMContext& llctx) {
  staticMembers.push_back(new StaticMember(this, name, type, variability, initial, visibility));
}

TypeKind CoreType::typeKind() const { return TypeKind::core; }

String CoreType::toString() const { return getFullName(); }

GenericCoreType::GenericCoreType(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                 ast::DefineCoreType* _defineCoreType, QatModule* _parent,
                                 const VisibilityInfo& _visibInfo)
    : EntityOverview("genericCoreType",
                     Json()
                         ._("name", _name.value)
                         ._("fullName", _parent->getFullNameWithChild(_name.value))
                         ._("visibility", _visibInfo)
                         ._("moduleID", _parent->getID()),
                     _name.range),
      name(std::move(_name)), generics(_generics), defineCoreType(_defineCoreType), parent(_parent),
      visibility(_visibInfo) {
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

CoreType* GenericCoreType::fillGenerics(Vec<GenericToFill*>& types, IR::Context* ctx, FileRange range) {
  for (auto var : variants) {
    if (var.check([&](const String& msg, const FileRange& rng) { ctx->Error(msg, rng); }, types)) {
      return var.get();
    }
  }
  IR::fillGenerics(ctx, generics, types, range);
  auto variantName = IR::Logic::getGenericVariantName(name.value, types);
  defineCoreType->setVariantName(variantName);
  ctx->addActiveGeneric(IR::GenericEntityMarker{variantName, IR::GenericEntityType::coreType, range}, true);
  (void)defineCoreType->define(ctx);
  (void)defineCoreType->emit(ctx);
  auto* cTy = defineCoreType->getCoreType();
  variants.push_back(GenericVariant<CoreType>(cTy, types));
  for (auto* temp : generics) {
    temp->unset();
  }
  defineCoreType->unsetVariantName();
  if (ctx->getActiveGeneric().warningCount > 0) {
    auto count = ctx->getActiveGeneric().warningCount;
    ctx->removeActiveGeneric();
    ctx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                     " generated while creating generic variant " + ctx->highlightWarning(variantName),
                 range);
  } else {
    ctx->removeActiveGeneric();
  }
  return cTy;
}

} // namespace qat::IR