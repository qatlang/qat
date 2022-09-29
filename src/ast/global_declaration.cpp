#include "./global_declaration.hpp"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"

namespace qat::ast {

GlobalDeclaration::GlobalDeclaration(String _name, QatType *_type,
                                     Expression *_value, bool _isVariable,
                                     utils::VisibilityKind _kind,
                                     utils::FileRange      _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), type(_type),
      value(_value), isVariable(_isVariable), visibility(_kind) {}

void GlobalDeclaration::define(IR::Context *ctx) {
  auto *mod  = ctx->getMod();
  auto *init = mod->getGlobalInitialiser(ctx);
  ctx->fn    = init;
  init->getBlock()->setActive(ctx->builder);
  auto                 *typ  = type->emit(ctx);
  auto                 *val  = value->emit(ctx);
  llvm::GlobalVariable *gvar = nullptr;
  if (llvm::isa<llvm::Constant>(val->getLLVM())) {
    gvar = new llvm::GlobalVariable(
        *mod->getLLVMModule(), typ->getLLVMType(), isVariable,
        visibility == utils::VisibilityKind::pub
            ? llvm::GlobalValue::LinkageTypes::WeakAnyLinkage
            : llvm::GlobalValue::LinkageTypes::PrivateLinkage,
        llvm::dyn_cast<llvm::Constant>(val->getLLVM()), name);
  } else {
    gvar = new llvm::GlobalVariable(
        *mod->getLLVMModule(), typ->getLLVMType(), false,
        visibility == utils::VisibilityKind::pub
            ? llvm::GlobalValue::LinkageTypes::WeakAnyLinkage
            : llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage,
        llvm::Constant::getNullValue(typ->getLLVMType()));
    ctx->builder.CreateStore(val->getLLVM(), gvar);
  }
  globalEntity = new IR::GlobalEntity(mod, name, type->emit(ctx), isVariable,
                                      gvar, ctx->getVisibInfo(visibility));
  ctx->fn      = nullptr;
}

IR::Value *GlobalDeclaration::emit(IR::Context *ctx) { return globalEntity; }

Json GlobalDeclaration::toJson() const {
  return Json()
      ._("nodeType", "globalDeclaration")
      ._("name", name)
      ._("type", type->toJson())
      ._("value", value->toJson())
      ._("variability", isVariable)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast