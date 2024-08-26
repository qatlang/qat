#include "./c_type.hpp"

namespace qat::ast {

ir::Type* CType::emit(EmitCtx* ctx) {
  switch (cTypeKind) {
    case ir::CTypeKind::String:
      return ir::CType::get_cstr(ctx->irCtx);
    case ir::CTypeKind::Bool:
      return ir::CType::get_bool(ctx->irCtx);
    case ir::CTypeKind::Int:
      return ir::CType::get_int(ctx->irCtx);
    case ir::CTypeKind::Uint:
      return ir::CType::get_uint(ctx->irCtx);
    case ir::CTypeKind::Char:
      return ir::CType::get_char(ctx->irCtx);
    case ir::CTypeKind::UChar:
      return ir::CType::get_char_unsigned(ctx->irCtx);
    case ir::CTypeKind::Short:
      return ir::CType::get_short(ctx->irCtx);
    case ir::CTypeKind::UShort:
      return ir::CType::get_short_unsigned(ctx->irCtx);
    case ir::CTypeKind::WideChar:
      return ir::CType::get_wide_char(ctx->irCtx);
    case ir::CTypeKind::UWideChar:
      return ir::CType::get_wide_char_unsigned(ctx->irCtx);
    case ir::CTypeKind::LongInt:
      return ir::CType::get_long_int(ctx->irCtx);
    case ir::CTypeKind::ULongInt:
      return ir::CType::get_long_int_unsigned(ctx->irCtx);
    case ir::CTypeKind::LongLong:
      return ir::CType::get_long_long(ctx->irCtx);
    case ir::CTypeKind::ULongLong:
      return ir::CType::get_long_long_unsigned(ctx->irCtx);
    case ir::CTypeKind::Usize:
      return ir::CType::get_usize(ctx->irCtx);
    case ir::CTypeKind::Isize:
      return ir::CType::get_isize(ctx->irCtx);
    case ir::CTypeKind::Float:
      return ir::CType::get_float(ctx->irCtx);
    case ir::CTypeKind::Double:
      return ir::CType::get_double(ctx->irCtx);
    case ir::CTypeKind::IntMax:
      return ir::CType::get_intmax(ctx->irCtx);
    case ir::CTypeKind::UintMax:
      return ir::CType::get_uintmax(ctx->irCtx);
    case ir::CTypeKind::IntPtr:
      return ir::CType::get_intptr(ctx->irCtx);
    case ir::CTypeKind::UintPtr:
      return ir::CType::get_uintptr(ctx->irCtx);
    case ir::CTypeKind::PtrDiff:
      return ir::CType::get_ptrdiff(ctx->irCtx);
    case ir::CTypeKind::UPtrDiff:
      return ir::CType::get_ptrdiff_unsigned(ctx->irCtx);
    case ir::CTypeKind::Pointer: {
      auto subTy = subType->emit(ctx);
      // TODO - Verify that the sub type is linked to have C compatible name
      return ir::CType::get_ptr(isPointerSubTypeVariable, subTy, ctx->irCtx);
    }
    case ir::CTypeKind::SigAtomic:
      return ir::CType::get_sigatomic(ctx->irCtx);
    case ir::CTypeKind::ProcessID:
      return ir::CType::get_processid(ctx->irCtx);
    case ir::CTypeKind::LongDouble: {
      if (ir::CType::has_long_double(ctx->irCtx)) {
        return ir::CType::get_long_double(ctx->irCtx);
      } else {
        ctx->Error("The target system for compilation does not support " + ctx->color(to_string()), fileRange);
      }
    }
  }
  ctx->Error("Compiler Internal Error - Cannot retrieve CType for kind " + std::to_string((int)cTypeKind), fileRange);
}

Maybe<usize> CType::getTypeSizeInBits(EmitCtx* ctx) const {
  switch (cTypeKind) {
    case ir::CTypeKind::String:
      return ctx->irCtx->clangTargetInfo->getPointerWidth(ctx->irCtx->get_language_address_space());
    case ir::CTypeKind::Bool:
      return ctx->irCtx->clangTargetInfo->getBoolWidth();
    case ir::CTypeKind::Int:
    case ir::CTypeKind::Uint:
      return ctx->irCtx->clangTargetInfo->getIntWidth();
    case ir::CTypeKind::Char:
    case ir::CTypeKind::UChar:
      return ctx->irCtx->clangTargetInfo->getCharWidth();
    case ir::CTypeKind::Short:
    case ir::CTypeKind::UShort:
      return ctx->irCtx->clangTargetInfo->getShortWidth();
    case ir::CTypeKind::WideChar:
    case ir::CTypeKind::UWideChar:
      return ctx->irCtx->clangTargetInfo->getWCharWidth();
    case ir::CTypeKind::LongInt:
    case ir::CTypeKind::ULongInt:
      return ctx->irCtx->clangTargetInfo->getLongWidth();
    case ir::CTypeKind::LongLong:
    case ir::CTypeKind::ULongLong:
      return ctx->irCtx->clangTargetInfo->getLongLongWidth();
    case ir::CTypeKind::Usize:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSizeType());
    case ir::CTypeKind::Isize:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSignedSizeType());
    case ir::CTypeKind::Float:
      return ctx->irCtx->clangTargetInfo->getFloatWidth();
    case ir::CTypeKind::Double:
      return ctx->irCtx->clangTargetInfo->getDoubleWidth();
    case ir::CTypeKind::IntMax:
    case ir::CTypeKind::UintMax:
      return ctx->irCtx->clangTargetInfo->getIntMaxTWidth();
    case ir::CTypeKind::IntPtr:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getIntPtrType());
    case ir::CTypeKind::UintPtr:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getUIntPtrType());
    case ir::CTypeKind::PtrDiff:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(
          ctx->irCtx->clangTargetInfo->getPtrDiffType(ctx->irCtx->get_language_address_space()));
    case ir::CTypeKind::UPtrDiff:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(
          ctx->irCtx->clangTargetInfo->getUnsignedPtrDiffType(ctx->irCtx->get_language_address_space()));
    case ir::CTypeKind::Pointer:
      return ctx->irCtx->clangTargetInfo->getPointerWidth(ctx->irCtx->get_language_address_space());
    case ir::CTypeKind::SigAtomic:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSigAtomicType());
    case ir::CTypeKind::ProcessID:
      return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getProcessIDType());
    case ir::CTypeKind::LongDouble: {
      if (ctx->irCtx->clangTargetInfo->hasLongDoubleType()) {
        return ctx->irCtx->clangTargetInfo->getLongDoubleWidth();
      }
      return None;
    }
  }
}

Json CType::to_json() const {
  return Json()._("typeKind", "cType")._("cTypeKind", ir::ctype_kind_to_string(cTypeKind))._("fileRange", fileRange);
}

String CType::to_string() const { return ir::ctype_kind_to_string(cTypeKind); }

} // namespace qat::ast
