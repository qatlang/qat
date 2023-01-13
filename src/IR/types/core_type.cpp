#include "core_type.hpp"
#include "../../ast/define_core_type.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "expanded_type.hpp"
#include "qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <utility>

namespace qat::IR {

CoreType::CoreType(QatModule* mod, Identifier _name, Vec<Member*> _members, const utils::VisibilityInfo& _visibility,
                   llvm::LLVMContext& ctx)
    : ExpandedType(std::move(_name), mod, _visibility), EntityOverview("coreType", Json(), _name.range),
      members(std::move(_members)) {
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

void CoreType::addStaticMember(const Identifier& name, QatType* type, bool variability, Value* initial,
                               const utils::VisibilityInfo& visibility, llvm::LLVMContext& ctx) {
  staticMembers.push_back(new StaticMember(this, name, type, variability, initial, visibility));
}

TypeKind CoreType::typeKind() const { return TypeKind::core; }

String CoreType::toString() const { return getFullName(); }

Json CoreType::toJson() const { return Json()._("id", getID())._("name", name); }

TemplateCoreType::TemplateCoreType(Identifier _name, Vec<ast::GenericAbstractType*> _templates,
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

Vec<IR::QatType*> TemplateCoreType::getDefaults(IR::Context* ctx) const {
  Vec<IR::QatType*> result;
  for (auto* typ : templates) {
    result.push_back(typ->emit(ctx));
  }
  return result;
}

bool TemplateCoreType::allTypesHaveDefaults() const {
  for (auto typ : templates) {
    if (!typ->hasDefault()) {
      return false;
    }
  }
  return true;
}

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