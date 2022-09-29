#include "./to_conversion.hpp"
#include "../../IR/types/cstring.hpp"
#include "../../IR/types/string_slice.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

IR::Value *ToConversion::emit(IR::Context *ctx) {
  auto *val = source->emit(ctx);
  // FIXME - Support references
  auto *destTy = destinationType->emit(ctx);
  if (val->getType()->isSame(destTy)) {
    if (val->getType()->isDefinition() || destTy->isDefinition()) {
      return new IR::Value(val->getLLVM(), destTy, false,
                           IR::Nature::temporary);
    } else {
      ctx->Warning(
          "Unnecessary conversion here. The expression is already of type " +
              ctx->highlightWarning(destTy->toString()) +
              ". Please remove this to avoid clutter.",
          fileRange);
      return val;
    }
  } else {
    auto *typ = val->getType();
    if (typ->isPointer()) {
      if (destTy->isPointer()) {
        return new IR::Value(ctx->builder.CreatePointerCast(
                                 val->getLLVM(), destTy->getLLVMType()),
                             destTy, false, IR::Nature::temporary);
      } else {
        ctx->Error("Pointer to other types not supported as of now", fileRange);
      }
    } else if (typ->isStringSlice()) {
      if (destTy->isCString()) {
        SHOW("Conversion from StringSlice to CString")
        if (llvm::isa<llvm::Constant>(val->getLLVM())) {
          SHOW("String slice is a constant struct")
          val = val->createAlloca(ctx->builder);
        }
        if (llvm::isa<llvm::AllocaInst>(val->getLLVM())) {
          return new IR::Value(
              ctx->builder.CreateLoad(
                  IR::CStringType::get(ctx->llctx)->getLLVMType(),
                  ctx->builder.CreateStructGEP(
                      IR::StringSliceType::get(ctx->llctx)->getLLVMType(),
                      val->getLLVM(), 0)),
              IR::CStringType::get(ctx->llctx), false, IR::Nature::temporary);
        } else {
          ctx->Error("Invalid value for String slice", fileRange);
        }
      }
      // TODO - Implement
    } else if (typ->isInteger()) {
      val->loadImplicitPointer(ctx->builder);
      if (destTy->isInteger()) {
        return new IR::Value(ctx->builder.CreateIntCast(
                                 val->getLLVM(), destTy->getLLVMType(), true),
                             destTy, val->isVariable(), val->getNature());
      } else if (destTy->isUnsignedInteger()) {
        ctx->Warning(
            "Conversion from signed integer to unsigned integers can be lossy",
            fileRange);
        if (typ->asInteger()->getBitwidth() ==
            destTy->asUnsignedInteger()->getBitwidth()) {
          return new IR::Value(val->getLLVM(), destTy, false, IR::Nature::pure);
        } else {
          return new IR::Value(ctx->builder.CreateIntCast(
                                   val->getLLVM(), destTy->getLLVMType(), true),
                               destTy, false, IR::Nature::temporary);
        }
      } else if (destTy->isFloat()) {
        return new IR::Value(
            ctx->builder.CreateSIToFP(val->getLLVM(), destTy->getLLVMType()),
            destTy, false, IR::Nature::temporary);
      }
      // TODO - Implement
    } else if (typ->isUnsignedInteger()) {
      val->loadImplicitPointer(ctx->builder);
      if (destTy->isUnsignedInteger()) {
        return new IR::Value(ctx->builder.CreateIntCast(
                                 val->getLLVM(), destTy->getLLVMType(), false),
                             destTy, false, IR::Nature::temporary);
      } else if (destTy->isInteger()) {
        ctx->Warning(
            "Conversion from unsigned integer to signed integers can be lossy",
            fileRange);
        if (typ->asUnsignedInteger()->getBitwidth() ==
            destTy->asInteger()->getBitwidth()) {
          return new IR::Value(val->getLLVM(), destTy, false,
                               IR::Nature::temporary);
        } else {
          return new IR::Value(ctx->builder.CreateIntCast(
                                   val->getLLVM(), destTy->getLLVMType(), true),
                               destTy, false, IR::Nature::temporary);
        }
      } else if (destTy->isFloat()) {
        return new IR::Value(
            ctx->builder.CreateUIToFP(val->getLLVM(), destTy->getLLVMType()),
            destTy, false, IR::Nature::temporary);
      }
      // TODO - Implement
    } else if (typ->isFloat()) {
      val->loadImplicitPointer(ctx->builder);
      if (destTy->isFloat()) {
        return new IR::Value(
            ctx->builder.CreateFPCast(val->getLLVM(), destTy->getLLVMType()),
            destTy, false, IR::Nature::temporary);
      } else if (destTy->isInteger()) {
        return new IR::Value(
            ctx->builder.CreateFPToSI(val->getLLVM(), destTy->getLLVMType()),
            destTy, false, IR::Nature::temporary);
      } else if (destTy->isUnsignedInteger()) {
        return new IR::Value(
            ctx->builder.CreateFPToUI(val->getLLVM(), destTy->getLLVMType()),
            destTy, false, IR::Nature::temporary);
      }
      // TODO - Implement
    }
  }
}

Json ToConversion::toJson() const {
  return Json()
      ._("nodeType", "toConversion")
      ._("instance", source->toJson())
      ._("targetType", destinationType->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast