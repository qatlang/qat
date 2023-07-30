#include "./c_type.hpp"

namespace qat::ast {

CType::CType(IR::CTypeKind _cTypeKind, FileRange _fileRange)
    : QatType(false, std::move(_fileRange)), cTypeKind(_cTypeKind) {}

CType::CType(QatType* _pointerSubTy, bool _isSubTyVar, FileRange _fileRange)
    : QatType(false, std::move(_fileRange)), cTypeKind(IR::CTypeKind::Pointer), subType(_pointerSubTy),
      isPointerSubTypeVariable(_isSubTyVar) {}

IR::QatType* CType::emit(IR::Context* ctx) {
  switch (cTypeKind) {
    case IR::CTypeKind::String:
      return IR::CType::getCString(ctx);
    case IR::CTypeKind::Bool:
      return IR::CType::getBool(ctx);
    case IR::CTypeKind::Int:
      return IR::CType::getInt(ctx);
    case IR::CTypeKind::Uint:
      return IR::CType::getUInt(ctx);
    case IR::CTypeKind::Char:
      return IR::CType::getChar(ctx);
    case IR::CTypeKind::UChar:
      return IR::CType::getCharUnsigned(ctx);
    case IR::CTypeKind::WideChar:
      return IR::CType::getWideChar(ctx);
    case IR::CTypeKind::UWideChar:
      return IR::CType::getWideCharUnsigned(ctx);
    case IR::CTypeKind::LongInt:
      return IR::CType::getLongInt(ctx);
    case IR::CTypeKind::ULongInt:
      return IR::CType::getLongIntUnsigned(ctx);
    case IR::CTypeKind::LongLong:
      return IR::CType::getLongLong(ctx);
    case IR::CTypeKind::ULongLong:
      return IR::CType::getLongLongUnsigned(ctx);
    case IR::CTypeKind::Usize:
      return IR::CType::getUsize(ctx);
    case IR::CTypeKind::Isize:
      return IR::CType::getIsize(ctx);
    case IR::CTypeKind::Float:
      return IR::CType::getFloat(ctx);
    case IR::CTypeKind::Double:
      return IR::CType::getDouble(ctx);
    case IR::CTypeKind::IntMax:
      return IR::CType::getIntMax(ctx);
    case IR::CTypeKind::UintMax:
      return IR::CType::getUintMax(ctx);
    case IR::CTypeKind::IntPtr:
      return IR::CType::getIntPtr(ctx);
    case IR::CTypeKind::UintPtr:
      return IR::CType::getUintPtr(ctx);
    case IR::CTypeKind::PtrDiff:
      return IR::CType::getPtrDiff(ctx);
    case IR::CTypeKind::UPtrDiff:
      return IR::CType::getPtrDiffUnsigned(ctx);
    case IR::CTypeKind::Pointer: {
      auto subTy = subType->emit(ctx);
      // TODO - Verify that the sub type is linked to have C compatible name
      return IR::CType::getPointer(isPointerSubTypeVariable, subTy, ctx);
    }
    case IR::CTypeKind::SigAtomic:
      return IR::CType::getSigAtomic(ctx);
    case IR::CTypeKind::ProcessID:
      return IR::CType::getProcessID(ctx);
    case IR::CTypeKind::HalfFloat: {
      if (IR::CType::hasHalfFloat(ctx)) {
        return IR::CType::getHalfFloat(ctx);
      } else {
        ctx->Error("The target system for compilation does not support " + ctx->highlightError(toString()), fileRange);
      }
    }
    case IR::CTypeKind::BrainFloat: {
      if (IR::CType::hasBrainFloat(ctx)) {
        return IR::CType::getBrainFloat(ctx);
      } else {
        ctx->Error("The target system for compilation does not support " + ctx->highlightError(toString()), fileRange);
      }
    }
    case IR::CTypeKind::Float128: {
      if (IR::CType::hasFloat128(ctx)) {
        return IR::CType::getFloat128(ctx);
      } else {
        ctx->Error("The target system for compilation does not support " + ctx->highlightError(toString()), fileRange);
      }
    }
    case IR::CTypeKind::LongDouble: {
      if (IR::CType::hasLongDouble(ctx)) {
        return IR::CType::getLongDouble(ctx);
      } else {
        ctx->Error("The target system for compilation does not support " + ctx->highlightError(toString()), fileRange);
      }
    }
  }
}

Maybe<usize> CType::getTypeSizeInBits(IR::Context* ctx) const {
  switch (cTypeKind) {
    case IR::CTypeKind::String:
      return ctx->clangTargetInfo->getPointerWidth(ctx->clangTargetInfo->getProgramAddressSpace());
    case IR::CTypeKind::Bool:
      return ctx->clangTargetInfo->getBoolWidth();
    case IR::CTypeKind::Int:
    case IR::CTypeKind::Uint:
      return ctx->clangTargetInfo->getIntWidth();
    case IR::CTypeKind::Char:
    case IR::CTypeKind::UChar:
      return ctx->clangTargetInfo->getCharWidth();
    case IR::CTypeKind::WideChar:
    case IR::CTypeKind::UWideChar:
      return ctx->clangTargetInfo->getWCharWidth();
    case IR::CTypeKind::LongInt:
    case IR::CTypeKind::ULongInt:
      return ctx->clangTargetInfo->getLongWidth();
    case IR::CTypeKind::LongLong:
    case IR::CTypeKind::ULongLong:
      return ctx->clangTargetInfo->getLongLongWidth();
    case IR::CTypeKind::Usize:
      return ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSizeType());
    case IR::CTypeKind::Isize:
      return ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSignedSizeType());
    case IR::CTypeKind::Float:
      return ctx->clangTargetInfo->getFloatWidth();
    case IR::CTypeKind::Double:
      return ctx->clangTargetInfo->getDoubleWidth();
    case IR::CTypeKind::IntMax:
    case IR::CTypeKind::UintMax:
      return ctx->clangTargetInfo->getIntMaxTWidth();
    case IR::CTypeKind::IntPtr:
      return ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getIntPtrType());
    case IR::CTypeKind::UintPtr:
      return ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getUIntPtrType());
    case IR::CTypeKind::PtrDiff:
      return ctx->clangTargetInfo->getTypeWidth(
          ctx->clangTargetInfo->getPtrDiffType(ctx->clangTargetInfo->getProgramAddressSpace()));
    case IR::CTypeKind::UPtrDiff:
      return ctx->clangTargetInfo->getTypeWidth(
          ctx->clangTargetInfo->getUnsignedPtrDiffType(ctx->clangTargetInfo->getProgramAddressSpace()));
    case IR::CTypeKind::Pointer:
      return ctx->clangTargetInfo->getPointerWidth(ctx->clangTargetInfo->getProgramAddressSpace());
    case IR::CTypeKind::SigAtomic:
      return ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSigAtomicType());
    case IR::CTypeKind::ProcessID:
      return ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getProcessIDType());
    case IR::CTypeKind::HalfFloat: {
      if (ctx->clangTargetInfo->hasLegalHalfType()) {
        return ctx->clangTargetInfo->getHalfWidth();
      }
      return None;
    }
    case IR::CTypeKind::BrainFloat: {
      if (ctx->clangTargetInfo->hasBFloat16Type()) {
        return ctx->clangTargetInfo->getBFloat16Width();
      }

      return None;
    }
    case IR::CTypeKind::Float128: {
      if (ctx->clangTargetInfo->hasFloat128Type()) {
        return ctx->clangTargetInfo->getFloat128Width();
      }
      return None;
    }
    case IR::CTypeKind::LongDouble: {
      if (ctx->clangTargetInfo->hasLongDoubleType()) {
        return ctx->clangTargetInfo->getLongDoubleWidth();
      }
      return None;
    }
  }
}

Json CType::toJson() const {
  return Json()._("typeKind", "cType")._("cTypeKind", IR::cTypeKindToString(cTypeKind))._("fileRange", fileRange);
}

String CType::toString() const { return "type:c" + IR::cTypeKindToString(cTypeKind); }

} // namespace qat::ast