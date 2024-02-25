#include "./global_declaration.hpp"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"

namespace qat::ast {

void GlobalDeclaration::create_entity(IR::QatModule* mod, IR::Context* ctx) {
  SHOW("CreateEntity: " << name.value)
  mod->entity_name_check(ctx, name, IR::EntityType::global);
  entityState = mod->add_entity(name, IR::EntityType::global, this, IR::EmitPhase::phase_1);
}

void GlobalDeclaration::update_entity_dependencies(IR::QatModule* parent, IR::Context* ctx) {
  type->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
  if (value.has_value()) {
    value.value()->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
  }
}

void GlobalDeclaration::do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) { define(parent, ctx); }

void GlobalDeclaration::define(IR::QatModule* mod, IR::Context* ctx) {
  ctx->nameCheckInModule(name, "global entity", None);
  auto visibInfo = ctx->getVisibInfo(visibSpec);
  if (!type) {
    ctx->Error("Expected a type for global declaration", fileRange);
  }
  auto*                  typ  = type->emit(ctx);
  llvm::GlobalVariable*  gvar = nullptr;
  Maybe<llvm::Constant*> initialValue;
  Maybe<String>          foreignID;
  Maybe<String>          linkAlias;
  Maybe<IR::MetaInfo>    irMetaInfo;
  if (metaInfo.has_value()) {
    irMetaInfo = metaInfo.value().toIR(ctx);
    foreignID  = irMetaInfo.value().getValueAsStringFor("foreign");
    linkAlias  = irMetaInfo.value().getValueAsStringFor(IR::MetaInfo::linkAsKey);
  }
  if (!foreignID.has_value()) {
    foreignID = mod->getRelevantForeignID();
  }
  auto linkNames = mod->getLinkNames().newWith(LinkNameUnit(name.value, LinkUnitType::global), foreignID);
  linkNames.setLinkAlias(linkAlias);
  auto linkingName = linkNames.toName();
  SHOW("Global is a variable: " << (isVariable ? "true" : "false"))
  if (value.has_value()) {
    // FIXME - Also do destruction logic
    auto* init  = mod->getGlobalInitialiser(ctx);
    auto* oldFn = ctx->setActiveFunction(init);
    init->getBlock()->setActive(ctx->builder);
    if (value.value()->isInPlaceCreatable()) {
      gvar = new llvm::GlobalVariable(
          *mod->getLLVMModule(), typ->getLLVMType(), false, llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
          typ->getLLVMType()->isPointerTy()
              ? llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(typ->getLLVMType()))
              : llvm::Constant::getNullValue(typ->getLLVMType()),
          linkingName);
      value.value()->asInPlaceCreatable()->setCreateIn(new IR::Value(gvar, typ, false, IR::Nature::temporary));
      (void)value.value()->emit(ctx);
      value.value()->asInPlaceCreatable()->unsetCreateIn();
    } else {
      auto val = value.value()->emit(ctx);
      if (val->isPrerunValue()) {
        gvar         = new llvm::GlobalVariable(*mod->getLLVMModule(), typ->getLLVMType(), !isVariable,
                                                ctx->getGlobalLinkageForVisibility(visibInfo),
                                                llvm::dyn_cast<llvm::Constant>(val->getLLVM()), linkingName);
        initialValue = val->getLLVMConstant();
      } else {
        if (typ->isReference()) {
          typ = typ->asReference()->getSubType();
        }
        mod->incrementNonConstGlobalCounter();
        gvar = new llvm::GlobalVariable(
            *mod->getLLVMModule(), typ->getLLVMType(), false, llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
            typ->getLLVMType()->isPointerTy()
                ? llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(typ->getLLVMType()))
                : llvm::Constant::getNullValue(typ->getLLVMType()),
            linkingName);
        if (val->isValue()) {
          ctx->builder.CreateStore(val->getLLVM(), gvar);
        } else {
          if (typ->isTriviallyCopyable() || typ->isTriviallyMovable()) {
            if (val->isReference()) {
              val->loadImplicitPointer(ctx->builder);
            }
            auto origVal = val;
            auto result  = ctx->builder.CreateLoad(typ->getLLVMType(), val->getLLVM());
            if (!typ->isTriviallyCopyable()) {
              if (origVal->isReference() ? origVal->getType()->asReference()->isSubtypeVariable()
                                         : origVal->isVariable()) {
                ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                           value.value()->fileRange);
              }
              ctx->builder.CreateStore(llvm::Constant::getNullValue(typ->getLLVMType()), origVal->getLLVM());
            }
            ctx->builder.CreateStore(result, gvar);
          } else {
            ctx->Error("This expression is a reference to the type " + ctx->highlightError(typ->toString()) +
                           " which is not trivially copyable or movable. Please use " + ctx->highlightError("'copy") +
                           " or " + ctx->highlightError("'move") + " accordingly",
                       fileRange);
          }
        }
      }
    }
    (void)ctx->setActiveFunction(oldFn);
  } else {
    if (!foreignID.has_value()) {
      ctx->Error(
          "This global entity is not a foreign entity, and not part of a foreign module, so it is required to have a value",
          fileRange);
    }
    gvar = new llvm::GlobalVariable(*mod->getLLVMModule(), typ->getLLVMType(), !isVariable,
                                    llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, nullptr, linkingName, nullptr,
                                    llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, None, true);
  }
  new IR::GlobalEntity(mod, name, type->emit(ctx), isVariable, initialValue, gvar, visibInfo);
}

Json GlobalDeclaration::toJson() const {
  return Json()
      ._("nodeType", "globalDeclaration")
      ._("name", name)
      ._("type", type ? type->toJson() : Json())
      ._("hasValue", value.has_value())
      ._("value", value ? value.value()->toJson() : JsonValue())
      ._("variability", isVariable)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec.value().toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast