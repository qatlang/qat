#include "./logic.hpp"
#include "context.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/reference.hpp"
#include "value.hpp"

namespace qat::IR {

String Logic::getTemplateVariantName(String mainName, Vec<IR::QatType*>& types) {
  String result(std::move(mainName));
  result += "'<";
  for (usize i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ">";
  return result;
}

llvm::AllocaInst* Logic::newAlloca(IR::Function* fun, const String& name, llvm::Type* type) {
  llvm::AllocaInst* result = nullptr;
  llvm::Function*   func   = fun->isAsyncFunction() ? fun->getAsyncSubFunction() : fun->getLLVMFunction();
  if (func->getEntryBlock().getInstList().empty()) {
    result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
  } else {
    llvm::Instruction* inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr = func->getEntryBlock().getInstList().begin(); instr != func->getEntryBlock().getInstList().end();
         instr++) {
      if (llvm::isa<llvm::AllocaInst>(&*instr)) {
        continue;
      } else {
        inst = &*instr;
        break;
      }
    }
    if (inst) {
      result = new llvm::AllocaInst(type, 0U, name, inst);
    } else {
      result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
    }
  }
  return result;
}

IR::Value* Logic::handleCopyOrMove(IR::Value* value, IR::Context* ctx, const utils::FileRange& fileRange) {
  if (value->getType()->isCoreType()) {
    if (value->isImplicitPointer() || value->isReference()) {
      auto* cTy = value->getType()->asCore();
      if (cTy->hasCopyConstructor()) {
        auto* cpFn = cTy->getCopyConstructor();
        auto* copy = newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
        (void)cpFn->call(ctx, {copy, value->getLLVM()}, ctx->getMod());
        return new IR::Value(copy, IR::ReferenceType::get(true, cTy, ctx->llctx), false, IR::Nature::temporary);
      } else if (cTy->hasMoveConstructor()) {
        auto* mvFn  = cTy->getMoveConstructor();
        auto* moved = newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
        (void)mvFn->call(ctx, {moved, value->getLLVM()}, ctx->getMod());
        ctx->Warning("A move is happening here. Considering making it explicit so that "
                     "there is no confusion as to the validity of the moved value",
                     fileRange);
        return new IR::Value(moved, IR::ReferenceType::get(true, cTy, ctx->llctx), false, IR::Nature::temporary);
      } else if (cTy->isTriviallyCopyable()) {
        return value;
      } else {
        ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) +
                       " has no copy or move constructors, and is also not "
                       "trivially copyable and hence cannot be "
                       "copied or moved",
                   fileRange);
      }
    } else {
      return value;
    }
  } else {
    ctx->Error("Only core types can be copied or moved", fileRange);
    // FIXME - Support mix types
  }
  // FIXME- Remove this
  return nullptr;
}

} // namespace qat::IR