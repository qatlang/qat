#include "./plain_initialiser.hpp"

namespace qat::ast {

PlainInitialiser::PlainInitialiser(NamedType                          *_type,
                                   Vec<Pair<String, utils::FileRange>> _fields,
                                   Vec<Expression *> _fieldValues,
                                   utils::FileRange  _fileRange)
    : Expression(std::move(_fileRange)), type(_type),
      fields(std::move(_fields)), fieldValues(std::move(_fieldValues)) {}

IR::Value *PlainInitialiser::emit(IR::Context *ctx) {
  if (type) {
    auto  reqInfo  = ctx->getReqInfo();
    auto *typeEmit = type->emit(ctx);
    if (typeEmit->isCoreType()) {
      auto *cTy = typeEmit->asCore();
      if (cTy->hasAnyNormalConstructor()) {
        ctx->Error(
            "Core type " + ctx->highlightError(cTy->getFullName()) +
                " has normal constructors and hence the plain initialiser "
                "cannot be used for this type",
            fileRange);
      }
      if (cTy->getMemberCount() != fieldValues.size()) {
        ctx->Error(
            "Core type " + ctx->highlightError(cTy->toString()) + " has " +
                ctx->highlightError(std::to_string(cTy->getMemberCount())) +
                " members. But there are only " +
                ctx->highlightError(std::to_string(fieldValues.size())) +
                " expressions provided",
            fileRange);
      }
      if (fields.empty()) {
        for (usize i = 0; i < fieldValues.size(); i++) {
          auto *mem = cTy->getMemberAt(i);
          if (!mem->visibility.isAccessible(reqInfo)) {
            ctx->Error("Member " + ctx->highlightError(mem->name) +
                           " of core type " +
                           ctx->highlightError(cTy->getFullName()) +
                           " is not accessible here and hence plain "
                           "initialiser cannot be used for this type",
                       fieldValues.at(i)->fileRange);
          }
          indices.push_back(i);
        }
      } else {
        for (usize i = 0; i < fields.size(); i++) {
          auto fName  = fields.at(i).first;
          auto fRange = fields.at(i).second;
          if (cTy->hasMember(fName)) {
            auto *mem = cTy->getMemberAt(cTy->getIndexOf(fName).value());
            if (!mem->visibility.isAccessible(reqInfo)) {
              ctx->Error("The member " + ctx->highlightError(fName) +
                             " of core type " +
                             ctx->highlightError(cTy->getFullName()) +
                             " is not accessible here",
                         fRange);
            }
            for (usize j = 0; j != i; j++) {
              if (fields.at(i).first == fields.at(j).first) {
                ctx->Error("The value for member " +
                               ctx->highlightError(fields.at(i).first) +
                               " of core type " +
                               ctx->highlightError(cTy->toString()) +
                               " is already provided. Please check logic",
                           fields.at(i).second);
              }
            }
            indices.push_back(cTy->getIndexOf(fields.at(i).first).value());
          } else {
            ctx->Error("The core type " + ctx->highlightError(cTy->toString()) +
                           " does not have a member named " +
                           ctx->highlightError(fields.at(i).first),
                       fields.at(i).second);
          }
        }
      }
      // FIXME - Support default values
      Vec<IR::Value *> irVals;
      for (usize i = 0; i < fieldValues.size(); i++) {
        auto *fVal  = fieldValues.at(i)->emit(ctx);
        auto *memTy = cTy->getMemberAt(i)->type;
        if (fVal->getType()->isSame(memTy)) {
          fVal->loadImplicitPointer(ctx->builder);
          irVals.push_back(fVal);
        } else if (memTy->isReference() &&
                   memTy->asReference()->getSubType()->isSame(
                       fVal->getType()) &&
                   fVal->isImplicitPointer() &&
                   (memTy->asReference()->isSubtypeVariable() ==
                    fVal->isVariable())) {
          irVals.push_back(fVal);
        } else if (fVal->getType()->isReference() &&
                   fVal->getType()->asReference()->getSubType()->isSame(
                       memTy)) {
          fVal = new IR::Value(
              ctx->builder.CreateLoad(memTy->getLLVMType(), fVal->getLLVM()),
              memTy, false, IR::Nature::temporary);
          irVals.push_back(fVal);
        } else {
          ctx->Error("This expression does not match the type of the "
                     "corresponding member " +
                         ctx->highlightError(cTy->getMemberNameAt(i)) +
                         " of core type " +
                         ctx->highlightError(cTy->toString()) +
                         ". The member is of type " +
                         ctx->highlightError(memTy->toString()) +
                         ", but the expression is of type " +
                         ctx->highlightError(fVal->getType()->toString()),
                     fieldValues.at(i)->fileRange);
        }
      }
      llvm::AllocaInst *alloca;
      if (local) {
        alloca = local->getAlloca();
      } else if (!irName.empty()) {
        local  = ctx->fn->getBlock()->newValue(irName, cTy, isVar);
        alloca = local->getAlloca();
      } else {
        alloca = ctx->builder.CreateAlloca(cTy->getLLVMType(), 0u);
      }
      for (usize i = 0; i < irVals.size(); i++) {
        auto *memPtr = ctx->builder.CreateStructGEP(cTy->getLLVMType(), alloca,
                                                    indices.at(i));
        ctx->builder.CreateStore(irVals.at(i)->getLLVM(), memPtr);
      }
      if (local) {
        return new IR::Value(local->getAlloca(), local->getType(),
                             local->isVariable(), local->getNature());
      } else {
        return new IR::Value(alloca, cTy, true, IR::Nature::pure);
      }
    } else {
      ctx->Error(
          "This type is not a core type and hence cannot be used in a plain "
          "initialiser",
          type->fileRange);
    }
  } else {
    ctx->Error("No type provided for plain initialisation", fileRange);
  }
  return nullptr;
}

nuo::Json PlainInitialiser::toJson() const {
  return nuo::Json()
      ._("nodeType", "plainInitialiser")
      ._("type", type->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast