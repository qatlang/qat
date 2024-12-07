#include "./native_type.hpp"
#include "../../IR/context.hpp"

namespace qat::ast {

ir::Type* NativeType::emit(EmitCtx* ctx) {
	switch (nativeKind) {
		case ir::NativeTypeKind::String:
			return ir::NativeType::get_cstr(ctx->irCtx);
		case ir::NativeTypeKind::Bool:
			return ir::NativeType::get_bool(ctx->irCtx);
		case ir::NativeTypeKind::Int:
			return ir::NativeType::get_int(ctx->irCtx);
		case ir::NativeTypeKind::Uint:
			return ir::NativeType::get_uint(ctx->irCtx);
		case ir::NativeTypeKind::Char:
			return ir::NativeType::get_char(ctx->irCtx);
		case ir::NativeTypeKind::UChar:
			return ir::NativeType::get_char_unsigned(ctx->irCtx);
		case ir::NativeTypeKind::Short:
			return ir::NativeType::get_short(ctx->irCtx);
		case ir::NativeTypeKind::UShort:
			return ir::NativeType::get_short_unsigned(ctx->irCtx);
		case ir::NativeTypeKind::WideChar:
			return ir::NativeType::get_wide_char(ctx->irCtx);
		case ir::NativeTypeKind::UWideChar:
			return ir::NativeType::get_wide_char_unsigned(ctx->irCtx);
		case ir::NativeTypeKind::LongInt:
			return ir::NativeType::get_long_int(ctx->irCtx);
		case ir::NativeTypeKind::ULongInt:
			return ir::NativeType::get_long_int_unsigned(ctx->irCtx);
		case ir::NativeTypeKind::LongLong:
			return ir::NativeType::get_long_long(ctx->irCtx);
		case ir::NativeTypeKind::ULongLong:
			return ir::NativeType::get_long_long_unsigned(ctx->irCtx);
		case ir::NativeTypeKind::Usize:
			return ir::NativeType::get_usize(ctx->irCtx);
		case ir::NativeTypeKind::Isize:
			return ir::NativeType::get_isize(ctx->irCtx);
		case ir::NativeTypeKind::Float:
			return ir::NativeType::get_float(ctx->irCtx);
		case ir::NativeTypeKind::Double:
			return ir::NativeType::get_double(ctx->irCtx);
		case ir::NativeTypeKind::IntMax:
			return ir::NativeType::get_intmax(ctx->irCtx);
		case ir::NativeTypeKind::UintMax:
			return ir::NativeType::get_uintmax(ctx->irCtx);
		case ir::NativeTypeKind::IntPtr:
			return ir::NativeType::get_intptr(ctx->irCtx);
		case ir::NativeTypeKind::UintPtr:
			return ir::NativeType::get_uintptr(ctx->irCtx);
		case ir::NativeTypeKind::PtrDiff:
			return ir::NativeType::get_ptrdiff(ctx->irCtx);
		case ir::NativeTypeKind::UPtrDiff:
			return ir::NativeType::get_ptrdiff_unsigned(ctx->irCtx);
		case ir::NativeTypeKind::Pointer: {
			auto subTy = subType->emit(ctx);
			// TODO - Verify that the sub type is linked to have C compatible name
			return ir::NativeType::get_ptr(isPointerSubTypeVariable, subTy, ctx->irCtx);
		}
		case ir::NativeTypeKind::SigAtomic:
			return ir::NativeType::get_sigatomic(ctx->irCtx);
		case ir::NativeTypeKind::ProcessID:
			return ir::NativeType::get_processid(ctx->irCtx);
		case ir::NativeTypeKind::LongDouble: {
			if (ir::NativeType::has_long_double(ctx->irCtx)) {
				return ir::NativeType::get_long_double(ctx->irCtx);
			} else {
				ctx->Error("The target system for compilation does not support " + ctx->color(to_string()), fileRange);
			}
		}
	}
	ctx->Error("Compiler Internal Error - Cannot retrieve native type for kind " + std::to_string((int)nativeKind),
			   fileRange);
}

Maybe<usize> NativeType::getTypeSizeInBits(EmitCtx* ctx) const {
	switch (nativeKind) {
		case ir::NativeTypeKind::String:
			return ctx->irCtx->clangTargetInfo->getPointerWidth(ctx->irCtx->get_language_address_space());
		case ir::NativeTypeKind::Bool:
			return ctx->irCtx->clangTargetInfo->getBoolWidth();
		case ir::NativeTypeKind::Int:
		case ir::NativeTypeKind::Uint:
			return ctx->irCtx->clangTargetInfo->getIntWidth();
		case ir::NativeTypeKind::Char:
		case ir::NativeTypeKind::UChar:
			return ctx->irCtx->clangTargetInfo->getCharWidth();
		case ir::NativeTypeKind::Short:
		case ir::NativeTypeKind::UShort:
			return ctx->irCtx->clangTargetInfo->getShortWidth();
		case ir::NativeTypeKind::WideChar:
		case ir::NativeTypeKind::UWideChar:
			return ctx->irCtx->clangTargetInfo->getWCharWidth();
		case ir::NativeTypeKind::LongInt:
		case ir::NativeTypeKind::ULongInt:
			return ctx->irCtx->clangTargetInfo->getLongWidth();
		case ir::NativeTypeKind::LongLong:
		case ir::NativeTypeKind::ULongLong:
			return ctx->irCtx->clangTargetInfo->getLongLongWidth();
		case ir::NativeTypeKind::Usize:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSizeType());
		case ir::NativeTypeKind::Isize:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSignedSizeType());
		case ir::NativeTypeKind::Float:
			return ctx->irCtx->clangTargetInfo->getFloatWidth();
		case ir::NativeTypeKind::Double:
			return ctx->irCtx->clangTargetInfo->getDoubleWidth();
		case ir::NativeTypeKind::IntMax:
		case ir::NativeTypeKind::UintMax:
			return ctx->irCtx->clangTargetInfo->getIntMaxTWidth();
		case ir::NativeTypeKind::IntPtr:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getIntPtrType());
		case ir::NativeTypeKind::UintPtr:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getUIntPtrType());
		case ir::NativeTypeKind::PtrDiff:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(
				ctx->irCtx->clangTargetInfo->getPtrDiffType(ctx->irCtx->get_language_address_space()));
		case ir::NativeTypeKind::UPtrDiff:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(
				ctx->irCtx->clangTargetInfo->getUnsignedPtrDiffType(ctx->irCtx->get_language_address_space()));
		case ir::NativeTypeKind::Pointer:
			return ctx->irCtx->clangTargetInfo->getPointerWidth(ctx->irCtx->get_language_address_space());
		case ir::NativeTypeKind::SigAtomic:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSigAtomicType());
		case ir::NativeTypeKind::ProcessID:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getProcessIDType());
		case ir::NativeTypeKind::LongDouble: {
			if (ctx->irCtx->clangTargetInfo->hasLongDoubleType()) {
				return ctx->irCtx->clangTargetInfo->getLongDoubleWidth();
			}
			return None;
		}
	}
}

Json NativeType::to_json() const {
	return Json()
		._("typeKind", "nativeType")
		._("cTypeKind", ir::native_type_kind_to_string(nativeKind))
		._("fileRange", fileRange);
}

String NativeType::to_string() const { return ir::native_type_kind_to_string(nativeKind); }

} // namespace qat::ast
