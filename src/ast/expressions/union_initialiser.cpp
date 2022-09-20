#include "./union_initialiser.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

UnionInitialiser::UnionInitialiser(u32 _relative, String _typeName,
                                   String              _subName,
                                   Maybe<Expression *> _expression,
                                   utils::FileRange    _fileRange)
    : Expression(std::move(_fileRange)), relative(_relative),
      typeName(std::move(_typeName)), subName(std::move(_subName)),
      expression(_expression) {}

IR::Value *UnionInitialiser::emit(IR::Context *ctx) {
  auto *mod     = ctx->getMod();
  auto  reqInfo = ctx->getReqInfo();
  if (relative != 0) {
    if (mod->hasNthParent(relative)) {
      mod = mod->getNthParent(relative);
    } else {
      ctx->Error("The current scope does not have " +
                     ctx->highlightError(std::to_string(relative)) +
                     " parents. Please check the logic",
                 fileRange);
    }
  }
  auto entityName = typeName;
  if (typeName.find(':') != String::npos) {
    auto splits = utils::splitString(typeName, ":");
    for (usize i = 0; i < (splits.size() - 1); i++) {
      auto split = splits.at(i);
      if (mod->hasLib(split)) {
        mod = mod->getLib(split, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
      } else if (mod->hasBox(split)) {
        mod = mod->getBox(split, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Box " + ctx->highlightError(mod->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
      } else {
        ctx->Error("No box or lib named " + ctx->highlightError(split) +
                       " found inside " +
                       ctx->highlightError(mod->getFullName()),
                   fileRange);
      }
    }
    entityName = splits.back();
  }
  if (mod->hasUnionType(entityName) || mod->hasBroughtUnionType(entityName) ||
      mod->hasAccessibleUnionTypeInImports(entityName, ctx->getReqInfo())
          .first) {
    auto *uTy = mod->getUnionType(entityName, ctx->getReqInfo());
    if (uTy->isAccessible(ctx->getReqInfo())) {
      auto subRes = uTy->hasSubTypeWithName(subName);
      if (subRes.first) {
        if (subRes.second) {
          if (!expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName) +
                           " of union type " +
                           ctx->highlightError(uTy->getFullName()) +
                           " expects a value to be associated with it",
                       fileRange);
          }
        } else {
          if (expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName) +
                           " of union type " +
                           ctx->highlightError(uTy->getFullName()) +
                           " cannot have any value associated with it",
                       fileRange);
          }
        }
        llvm::Value *exp = nullptr;
        if (subRes.second) {
          auto *typ     = uTy->getSubTypeWithName(subName);
          auto *expEmit = expression.value()->emit(ctx);
          if (typ->isSame(expEmit->getType())) {
            exp = expEmit->getLLVM();
          } else if (expEmit->isReference() &&
                     expEmit->getType()->asReference()->getSubType()->isSame(
                         typ)) {
            exp = ctx->builder.CreateLoad(
                expEmit->getType()->asReference()->getSubType()->getLLVMType(),
                expEmit->getLLVM());
          } else if (typ->isReference() &&
                     typ->asReference()->getSubType()->isSame(
                         expEmit->getType())) {
            if (expEmit->isImplicitPointer()) {
              exp = expEmit->getLLVM();
            } else {
              ctx->Error(
                  "The expected type is " +
                      ctx->highlightError(typ->toString()) +
                      ", but the expression is of type " +
                      ctx->highlightError(expEmit->getType()->toString()),
                  expression.value()->fileRange);
            }
          } else {
            ctx->Error("The expected type is " +
                           ctx->highlightError(typ->toString()) +
                           ", but the expression is of type " +
                           ctx->highlightError(expEmit->getType()->toString()),
                       expression.value()->fileRange);
          }
        } else {
          exp = llvm::ConstantInt::get(
              llvm::Type::getIntNTy(ctx->llctx, uTy->getDataBitwidth()), 0u,
              true);
        }
        auto *index = llvm::ConstantInt::get(
            llvm::Type::getIntNTy(ctx->llctx, uTy->getTagBitwidth()),
            uTy->getIndexOfName(subName));
        llvm::AllocaInst *alloca;
        if (local) {
          alloca = local->getAlloca();
        } else if (!irName.empty()) {
          local  = ctx->fn->getBlock()->newValue(irName, uTy, isVar);
          alloca = local->getAlloca();
        } else {
          alloca = ctx->builder.CreateAlloca(uTy->getLLVMType(), 0u);
        }
        SHOW("Creating union store")
        ctx->builder.CreateStore(
            index, ctx->builder.CreateStructGEP(uTy->getLLVMType(), alloca, 0));
        if (subRes.second) {
          ctx->builder.CreateStore(exp, ctx->builder.CreatePointerCast(
                                            ctx->builder.CreateStructGEP(
                                                uTy->getLLVMType(), alloca, 1),
                                            uTy->getSubTypeWithName(subName)
                                                ->getLLVMType()
                                                ->getPointerTo()));
        }
        if (local) {
          return new IR::Value(local->getAlloca(), local->getType(),
                               local->isVariable(), local->getNature());
        } else {
          return new IR::Value(alloca, uTy, true, IR::Nature::pure);
        }
      } else {
        ctx->Error("No field named " + ctx->highlightError(subName) +
                       " is present inside union type " +
                       ctx->highlightError(uTy->getFullName()),
                   fileRange);
      }
    } else {
      ctx->Error("Union type " + ctx->highlightError(uTy->getFullName()) +
                     " is not accessible here",
                 fileRange);
    }
  } else {
    ctx->Error("No union type named " + ctx->highlightError(entityName) +
                   " found inside module " +
                   ctx->highlightError(mod->getFullName()),
               fileRange);
  }
}

nuo::Json UnionInitialiser::toJson() const {
  return nuo::Json()
      ._("nodeType", "unionInitialiser")
      ._("relative", relative)
      ._("typeName", typeName)
      ._("subName", subName)
      ._("hasExpression", expression.has_value())
      ._("expression", expression.has_value() ? expression.value()->toJson()
                                              : nuo::JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast