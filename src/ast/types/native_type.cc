#include "./native_type.hpp"
#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

void NativeType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
	if (addressSpace.has_value() && addressSpace.value().value) {
		UPDATE_DEPS(addressSpace.value().value);
	}
}

ir::Type* NativeType::emit(EmitCtx* ctx) {
	switch (nativeKind) {
		case ir::NativeTypeKind::ByteString: {
			Maybe<ir::AddressSpace> addr = None;
			if (addressSpace.has_value()) {
				addr = addressSpace.value().to_ir(ctx);
			}
			return ir::NativeType::get_bytestring(varRange.has_value(), isNonNullable, std::move(addr), ctx->irCtx);
		}
		case ir::NativeTypeKind::Bool:
			return ir::NativeType::get_bool(ctx->irCtx);
		case ir::NativeTypeKind::Int:
			return ir::NativeType::get_int(ctx->irCtx);
		case ir::NativeTypeKind::Uint:
			return ir::NativeType::get_uint(ctx->irCtx);
		case ir::NativeTypeKind::Byte:
			return ir::NativeType::get_byte(ctx->irCtx);
		case ir::NativeTypeKind::UByte:
			return ir::NativeType::get_byte_unsigned(ctx->irCtx);
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
		case ir::NativeTypeKind::PtrDiff: {
			Maybe<ir::AddressSpace> addr;
			if (addressSpace.has_value()) {
				addr = addressSpace.value().to_ir(ctx);
			}
			return ir::NativeType::get_ptrdiff(std::move(addr), ctx->irCtx);
		}
		case ir::NativeTypeKind::UPtrDiff: {
			Maybe<ir::AddressSpace> addr;
			if (addressSpace.has_value()) {
				addr = addressSpace.value().to_ir(ctx);
			}
			return ir::NativeType::get_ptrdiff_unsigned(std::move(addr), ctx->irCtx);
		}
		case ir::NativeTypeKind::SigAtomic:
			return ir::NativeType::get_sigatomic(ctx->irCtx);
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
	return nullptr;
}

Maybe<usize> NativeType::get_type_bitsize(EmitCtx* ctx) const {
	switch (nativeKind) {
		case ir::NativeTypeKind::ByteString: {
			if (addressSpace.has_value() && addressSpace.value().value) {
				return None;
			}
			return ctx->irCtx->clangTargetInfo->getPointerWidth(clang::getLangASFromTargetAS(
			    addressSpace.has_value() ? addressSpace.value().to_ir(ctx).get_number(ctx->irCtx)
			                             : ctx->irCtx->dataLayout.getProgramAddressSpace()));
		}
		case ir::NativeTypeKind::Bool:
			return ctx->irCtx->clangTargetInfo->getBoolWidth();
		case ir::NativeTypeKind::Int:
		case ir::NativeTypeKind::Uint:
			return ctx->irCtx->clangTargetInfo->getIntWidth();
		case ir::NativeTypeKind::Byte:
		case ir::NativeTypeKind::UByte:
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
		case ir::NativeTypeKind::PtrDiff: {
			if (addressSpace.has_value() && addressSpace.value().value) {
				return None;
			}
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getPtrDiffType(
			    clang::getLangASFromTargetAS(addressSpace.has_value()
			                                     ? addressSpace.value().to_ir(ctx).get_number(ctx->irCtx)
			                                     : ctx->irCtx->dataLayout.getProgramAddressSpace())));
		}
		case ir::NativeTypeKind::UPtrDiff: {
			if (addressSpace.has_value() && addressSpace.value().value) {
				return None;
			}
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getUnsignedPtrDiffType(
			    clang::getLangASFromTargetAS(addressSpace.has_value()
			                                     ? addressSpace.value().to_ir(ctx).get_number(ctx->irCtx)
			                                     : ctx->irCtx->dataLayout.getProgramAddressSpace())));
		}
		case ir::NativeTypeKind::SigAtomic:
			return ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getSigAtomicType());
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
	    ._("nativeKind", ir::native_type_kind_to_string(nativeKind))
	    ._("isNonNullable", isNonNullable)
	    ._("hasVar", varRange.has_value())
	    ._("varRange", varRange.has_value() ? varRange.value()->to_json_value() : JsonValue())
	    ._("hasAddressSpace", addressSpace.has_value())
	    ._("addressSpace", addressSpace.has_value() ? addressSpace.value().to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

String NativeType::to_string() const {
	if (nativeKind == ir::NativeTypeKind::ByteString) {
		String res = "bytestring";
		if (isNonNullable) {
			res += "!";
		}
		if (varRange.has_value() || addressSpace.has_value()) {
			if (isNonNullable) {
				res += "[";
			}
			if (varRange.has_value()) {
				res += "var";
				if (addressSpace.has_value()) {
					res += ", ";
				}
			}
			if (addressSpace.has_value()) {
				res += addressSpace.value().to_string();
			}
			res += "]";
		}
		return res;
	} else if (nativeKind == ir::NativeTypeKind::UPtrDiff || nativeKind == ir::NativeTypeKind::PtrDiff) {
		String res = nativeKind == ir::NativeTypeKind::UPtrDiff ? "uptrdiff" : "ptrdiff";
		if (addressSpace.has_value()) {
			res += ":[" + addressSpace.value().to_string() + "]";
		}
		return res;
	}
	return ir::native_type_kind_to_string(nativeKind);
}

} // namespace qat::ast
