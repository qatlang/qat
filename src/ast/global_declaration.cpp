#include "./global_declaration.hpp"
#include "../memory_tracker.hpp"
#include "llvm/IR/Constant.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"

namespace qat::ast {

GlobalDeclaration::GlobalDeclaration(Identifier _name, QatType* _type, Expression* _value, bool _isVariable,
                                     utils::VisibilityKind _kind, FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), type(_type), value(_value), isVariable(_isVariable),
      visibility(_kind) {}

void GlobalDeclaration::define(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "global value", None);
  auto* init = mod->getGlobalInitialiser(ctx);
  ctx->fn    = init;
  init->getBlock()->setActive(ctx->builder);
  if (!type) {
    ctx->Error("Expected a type for global declaration", fileRange);
  }
  if (!value) {
    ctx->Error("Expected a value to be assigned for global declaration", fileRange);
  }
  auto*                  typ  = type->emit(ctx);
  auto*                  val  = value->emit(ctx);
  llvm::GlobalVariable*  gvar = nullptr;
  Maybe<llvm::Constant*> initialValue;
  SHOW("Global is a variable: " << (isVariable ? "true" : "false"))
  if (val->isConstVal()) {
    gvar = new llvm::GlobalVariable(
        *mod->getLLVMModule(), typ->getLLVMType(), !isVariable, llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
        llvm::dyn_cast<llvm::Constant>(val->getLLVM()), mod->getFullNameWithChild(name.value));
    initialValue = llvm::cast<llvm::Constant>(val->getLLVM());
  } else {
    mod->incrementNonConstGlobalCounter();
    gvar = new llvm::GlobalVariable(
        *mod->getLLVMModule(), typ->getLLVMType(), false, llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
        llvm::Constant::getNullValue(typ->getLLVMType()), mod->getFullNameWithChild(name.value));
    ctx->builder.CreateStore(val->getLLVM(), gvar);
  }
  globalEntity =
      new IR::GlobalEntity(mod, name, type->emit(ctx), isVariable, initialValue, gvar, ctx->getVisibInfo(visibility));
  ctx->fn = nullptr;
}

IR::Value* GlobalDeclaration::emit(IR::Context* ctx) { return globalEntity; }

Json GlobalDeclaration::toJson() const {
  return Json()
      ._("nodeType", "globalDeclaration")
      ._("name", name)
      ._("type", type ? type->toJson() : Json())
      ._("value", value ? value->toJson() : Json())
      ._("variability", isVariable)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast