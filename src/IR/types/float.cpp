#include "./float.hpp"
#include "../../memory_tracker.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../value.hpp"
#include "./c_type.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

namespace qat::IR {

FloatType::FloatType(FloatTypeKind _kind, llvm::LLVMContext& ctx) : kind(_kind) {
  switch (kind) {
    case FloatTypeKind::_brain: {
      llvmType = llvm::Type::getBFloatTy(ctx);
      break;
    }
    case FloatTypeKind::_16: {
      llvmType = llvm::Type::getHalfTy(ctx);
      break;
    }
    case FloatTypeKind::_32: {
      llvmType = llvm::Type::getFloatTy(ctx);
      break;
    }
    case FloatTypeKind::_64: {
      llvmType = llvm::Type::getDoubleTy(ctx);
      break;
    }
    case FloatTypeKind::_80: {
      llvmType = llvm::Type::getX86_FP80Ty(ctx);
      break;
    }
    case FloatTypeKind::_128PPC: {
      llvmType = llvm::Type::getPPC_FP128Ty(ctx);
      break;
    }
    case FloatTypeKind::_128: {
      llvmType = llvm::Type::getFP128Ty(ctx);
      break;
    }
  }
  linkingName = "qat'" + toString();
}

FloatType* FloatType::get(FloatTypeKind _kind, llvm::LLVMContext& llctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isFloat()) {
      if (typ->asFloat()->getKind() == _kind) {
        return typ->asFloat();
      }
    }
  }
  return new FloatType(_kind, llctx);
}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

bool FloatType::isTypeSized() const { return true; }

String FloatType::toString() const {
  switch (kind) {
    case FloatTypeKind::_brain: {
      return "fbrain";
    }
    case FloatTypeKind::_16: {
      return "f16";
    }
    case FloatTypeKind::_32: {
      return "f32";
    }
    case FloatTypeKind::_64: {
      return "f64";
    }
    case FloatTypeKind::_80: {
      return "f80";
    }
    case FloatTypeKind::_128: {
      return "f128";
    }
    case FloatTypeKind::_128PPC: {
      return "f128ppc";
    }
  }
}

Maybe<String> FloatType::toPrerunGenericString(IR::PrerunValue* val) const {
  if (val->getType()->isFloat()) {
    String                   strRef;
    llvm::raw_string_ostream stream(strRef);
    val->getLLVMConstant()->printAsOperand(stream);
    return strRef.find(' ') != String::npos ? strRef.substr(strRef.find(' ') + 1) : strRef;
  } else {
    return None;
  }
}

Maybe<bool> FloatType::equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType()) && first->getType()->isFloat()) {
    return llvm::cast<llvm::ConstantInt>(
               llvm::ConstantFoldConstant(llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OEQ, first->getLLVMConstant(),
                                                                      second->getLLVMConstant()),
                                          ctx->dataLayout.value()))
        ->getValue()
        .getBoolValue();
  } else {
    return None;
  }
}

} // namespace qat::IR