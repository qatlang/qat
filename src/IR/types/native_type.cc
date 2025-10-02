#include "./native_type.hpp"
#include "../context.hpp"
#include "./float.hpp"
#include "./integer.hpp"
#include "./pointer.hpp"
#include "./type_kind.hpp"
#include "./unsigned.hpp"

namespace qat::ir {

Maybe<NativeTypeKind> NativeType::kind_from_string(String const& val) {
	if (val == "bytestring") {
		return NativeTypeKind::ByteString;
	} else if (val == "int") {
		return NativeTypeKind::Int;
	} else if (val == "uint") {
		return NativeTypeKind::Uint;
	} else if (val == "byte") {
		return NativeTypeKind::Byte;
	} else if (val == "ubyte") {
		return NativeTypeKind::UByte;
	} else if (val == "shortint") {
		return NativeTypeKind::Short;
	} else if (val == "ushortint") {
		return NativeTypeKind::UShort;
	} else if (val == "widechar") {
		return NativeTypeKind::WideChar;
	} else if (val == "uwidechar") {
		return NativeTypeKind::UWideChar;
	} else if (val == "longint") {
		return NativeTypeKind::LongInt;
	} else if (val == "ulongint") {
		return NativeTypeKind::ULongInt;
	} else if (val == "longlong") {
		return NativeTypeKind::LongLong;
	} else if (val == "ulonglong") {
		return NativeTypeKind::ULongLong;
	} else if (val == "usize") {
		return NativeTypeKind::Usize;
	} else if (val == "isize") {
		return NativeTypeKind::Isize;
	} else if (val == "float") {
		return NativeTypeKind::Float;
	} else if (val == "double") {
		return NativeTypeKind::Double;
	} else if (val == "longdouble") {
		return NativeTypeKind::LongDouble;
	} else if (val == "intmax") {
		return NativeTypeKind::IntMax;
	} else if (val == "uintmax") {
		return NativeTypeKind::UintMax;
	} else if (val == "intptr") {
		return NativeTypeKind::IntPtr;
	} else if (val == "uintptr") {
		return NativeTypeKind::UintPtr;
	} else if (val == "ptrdiff") {
		return NativeTypeKind::PtrDiff;
	} else if (val == "uptrdiff") {
		return NativeTypeKind::UPtrDiff;
	} else if (val == "sigatomic") {
		return NativeTypeKind::SigAtomic;
	} else if (val == "widebool") {
		return NativeTypeKind::Bool;
	}
	return None;
}

String NativeType::kind_to_string(NativeTypeKind kind) {
	switch (kind) {
		case NativeTypeKind::Int:
			return "int";
		case NativeTypeKind::Uint:
			return "uint";
		case NativeTypeKind::Byte:
			return "byte";
		case NativeTypeKind::UByte:
			return "ubyte";
		case NativeTypeKind::Short:
			return "shortint";
		case NativeTypeKind::UShort:
			return "ushortint";
		case NativeTypeKind::WideChar:
			return "widechar";
		case NativeTypeKind::UWideChar:
			return "uwidechar";
		case NativeTypeKind::LongInt:
			return "longint";
		case NativeTypeKind::ULongInt:
			return "ulongint";
		case NativeTypeKind::LongLong:
			return "longlong";
		case NativeTypeKind::ULongLong:
			return "ulonglong";
		case NativeTypeKind::Usize:
			return "usize";
		case NativeTypeKind::Isize:
			return "isize";
		case NativeTypeKind::Float:
			return "float";
		case NativeTypeKind::Double:
			return "double";
		case NativeTypeKind::LongDouble:
			return "longdouble";
		case NativeTypeKind::IntMax:
			return "intmax";
		case NativeTypeKind::UintMax:
			return "uintmax";
		case NativeTypeKind::IntPtr:
			return "intptr";
		case NativeTypeKind::UintPtr:
			return "uintptr";
		case NativeTypeKind::PtrDiff:
			return "ptrdiff";
		case NativeTypeKind::UPtrDiff:
			return "uptrdiff";
		case NativeTypeKind::SigAtomic:
			return "sigatomic";
		case NativeTypeKind::ByteString:
			return "bytestring";
		case NativeTypeKind::Bool:
			return "widebool";
	}
}

Vec<NativeType*> NativeType::allNativeTypes = {};

NativeType::NativeType(ir::Type* actual, NativeTypeKind c_kind, Maybe<AddressSpace> _addressSpace)
    : subType(actual), nativeKind(c_kind), addressSpace(std::move(_addressSpace)) {
	llvmType    = actual->get_llvm_type();
	linkingName = "qat'nativetype:[" + to_string() + "]";
	allNativeTypes.push_back(this);
}

NativeTypeKind NativeType::get_c_type_kind() const { return nativeKind; }

ir::Type* NativeType::get_subtype() const { return subType; }

NativeType* NativeType::get_from_kind(NativeTypeKind kind, ir::Ctx* irCtx) {
	switch (kind) {
		case NativeTypeKind::ByteString:
			return get_bytestring(false, false, None, irCtx);
		case NativeTypeKind::Bool:
			return get_bool(irCtx);
		case NativeTypeKind::Int:
			return get_int(irCtx);
		case NativeTypeKind::Uint:
			return get_uint(irCtx);
		case NativeTypeKind::Byte:
			return get_byte(irCtx);
		case NativeTypeKind::UByte:
			return get_byte_unsigned(irCtx);
		case NativeTypeKind::Short:
			return get_short(irCtx);
		case NativeTypeKind::UShort:
			return get_short_unsigned(irCtx);
		case NativeTypeKind::WideChar:
			return get_wide_char(irCtx);
		case NativeTypeKind::UWideChar:
			return get_wide_char_unsigned(irCtx);
		case NativeTypeKind::LongInt:
			return get_long_int(irCtx);
		case NativeTypeKind::ULongInt:
			return get_long_int_unsigned(irCtx);
		case NativeTypeKind::LongLong:
			return get_long_long(irCtx);
		case NativeTypeKind::ULongLong:
			return get_long_long_unsigned(irCtx);
		case NativeTypeKind::Usize:
			return get_usize(irCtx);
		case NativeTypeKind::Isize:
			return get_isize(irCtx);
		case NativeTypeKind::Float:
			return get_float(irCtx);
		case NativeTypeKind::Double:
			return get_double(irCtx);
		case NativeTypeKind::IntMax:
			return get_intmax(irCtx);
		case NativeTypeKind::UintMax:
			return get_uintmax(irCtx);
		case NativeTypeKind::IntPtr:
			return get_intptr(irCtx);
		case NativeTypeKind::UintPtr:
			return get_uintptr(irCtx);
		case NativeTypeKind::PtrDiff:
			return get_ptrdiff(None, irCtx);
		case NativeTypeKind::UPtrDiff:
			return get_ptrdiff_unsigned(None, irCtx);
		case NativeTypeKind::SigAtomic:
			return get_sigatomic(irCtx);
		case NativeTypeKind::LongDouble:
			return get_long_double(irCtx);
	}
}

NativeType* NativeType::get_bool(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Bool) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getBoolWidth(), irCtx),
	                         NativeTypeKind::Bool);
}

NativeType* NativeType::get_int(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Int) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::IntegerType::get(irCtx->clangTargetInfo->getIntWidth(), irCtx),
	                         NativeTypeKind::Int);
}

NativeType* NativeType::get_uint(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Uint) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getIntWidth(), irCtx),
	                         NativeTypeKind::Uint);
}

NativeType* NativeType::get_byte(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Byte) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::IntegerType::get(irCtx->clangTargetInfo->getCharWidth(), irCtx),
	                         NativeTypeKind::Byte);
}

NativeType* NativeType::get_byte_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::UByte) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getCharWidth(), irCtx),
	                         NativeTypeKind::UByte);
}

NativeType* NativeType::get_short(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Short) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getShortWidth(), irCtx),
	                         NativeTypeKind::Byte);
}

NativeType* NativeType::get_short_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::UShort) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getShortWidth(), irCtx),
	                         NativeTypeKind::UByte);
}

NativeType* NativeType::get_wide_char(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::WideChar) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getWCharWidth(), irCtx),
	                         NativeTypeKind::WideChar);
}

NativeType* NativeType::get_wide_char_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::UWideChar) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getWCharWidth(), irCtx),
	                         NativeTypeKind::UWideChar);
}

NativeType* NativeType::get_long_int(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::LongInt) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::IntegerType::get(irCtx->clangTargetInfo->getLongWidth(), irCtx),
	                         NativeTypeKind::LongInt);
}

NativeType* NativeType::get_long_int_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::ULongInt) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getLongWidth(), irCtx),
	                         NativeTypeKind::ULongInt);
}

NativeType* NativeType::get_long_long(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::LongLong) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getLongLongWidth(), irCtx),
	                         NativeTypeKind::LongLong);
}

NativeType* NativeType::get_long_long_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::ULongLong) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getLongLongWidth(), irCtx),
	                         NativeTypeKind::ULongLong);
}

NativeType* NativeType::get_isize(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Isize) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSignedSizeType()), irCtx),
	    NativeTypeKind::Isize);
}

NativeType* NativeType::get_usize(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Usize) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::UnsignedType::create(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()), irCtx),
	    NativeTypeKind::Usize);
}

NativeType* NativeType::get_float(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Float) {
			return typ;
		}
	}
	FloatTypeKind floatKind = FloatTypeKind::_32;
	switch (irCtx->clangTargetInfo->getFloatWidth()) {
		case 128: {
			floatKind = FloatTypeKind::_128;
			break;
		}
		case 80: {
			floatKind = FloatTypeKind::_80;
			break;
		}
		case 64: {
			floatKind = FloatTypeKind::_64;
			break;
		}
		case 32: {
			floatKind = FloatTypeKind::_32;
			break;
		}
		case 16: {
			floatKind = FloatTypeKind::_16;
			break;
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::FloatType::get(floatKind, irCtx->llctx), NativeTypeKind::Float);
}

NativeType* NativeType::get_double(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::Double) {
			return typ;
		}
	}
	FloatTypeKind floatKind = FloatTypeKind::_64;
	switch (irCtx->clangTargetInfo->getDoubleWidth()) {
		case 128: {
			floatKind = FloatTypeKind::_128;
			break;
		}
		case 80: {
			floatKind = FloatTypeKind::_80;
			break;
		}
		case 64: {
			floatKind = FloatTypeKind::_64;
			break;
		}
		case 32: {
			floatKind = FloatTypeKind::_32;
			break;
		}
		case 16: {
			floatKind = FloatTypeKind::_16;
			break;
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::FloatType::get(floatKind, irCtx->llctx),
	                         NativeTypeKind::Double);
}

NativeType* NativeType::get_intmax(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::IntMax) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getIntMaxTWidth(), irCtx),
	                         NativeTypeKind::IntMax);
}

NativeType* NativeType::get_uintmax(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::UintMax) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getIntMaxTWidth(), irCtx),
	                         NativeTypeKind::UintMax);
}

NativeType* NativeType::get_intptr(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::IntPtr) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getIntPtrType()), irCtx),
	    NativeTypeKind::IntPtr);
}

NativeType* NativeType::get_uintptr(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::UintPtr) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::UnsignedType::create(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntMaxType()), irCtx),
	    NativeTypeKind::UintPtr);
}

NativeType* NativeType::get_ptrdiff(Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if ((typ->nativeKind == NativeTypeKind::PtrDiff) &&
		    ir::AddressSpace::compare(typ->addressSpace, addressSpace)) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(
	        irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getPtrDiffType(
	            clang::getLangASFromTargetAS(addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
	                                                                  : irCtx->dataLayout.getProgramAddressSpace()))),
	        irCtx),
	    NativeTypeKind::PtrDiff, std::move(addressSpace));
}

NativeType* NativeType::get_ptrdiff_unsigned(Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if ((typ->nativeKind == NativeTypeKind::UPtrDiff) &&
		    ir::AddressSpace::compare(typ->addressSpace, addressSpace)) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::UnsignedType::create(
	        irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUnsignedPtrDiffType(
	            clang::getLangASFromTargetAS(addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
	                                                                  : irCtx->dataLayout.getProgramAddressSpace()))),
	        irCtx),
	    NativeTypeKind::UPtrDiff, std::move(addressSpace));
}

NativeType* NativeType::get_sigatomic(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::SigAtomic) {
			return typ;
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSigAtomicType()), irCtx),
	    NativeTypeKind::SigAtomic);
}

NativeType* NativeType::get_bytestring(bool hasVar, bool isNonNullable, Maybe<AddressSpace> addressSpace,
                                       ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if ((typ->nativeKind == NativeTypeKind::ByteString) &&
		    (typ->get_subtype()->as_ptr()->is_subtype_variable() == hasVar) &&
		    (typ->get_subtype()->as_ptr()->is_non_nullable() == isNonNullable) &&
		    ir::AddressSpace::compare(typ->get_subtype()->as_ptr()->get_address_space(), addressSpace)) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::PtrType::get(hasVar, ir::IntegerType::get(8u, irCtx), isNonNullable,
	                                          PtrOwner::of_none(), false, addressSpace, irCtx),
	                         NativeTypeKind::ByteString, std::move(addressSpace));
}

bool NativeType::has_long_double(ir::Ctx* irCtx) { return irCtx->clangTargetInfo->hasLongDoubleType(); }

NativeType* NativeType::get_long_double(ir::Ctx* irCtx) {
	for (auto* typ : allNativeTypes) {
		if (typ->nativeKind == NativeTypeKind::LongDouble) {
			return typ;
		}
	}
	FloatTypeKind floatKind = FloatTypeKind::_80;
	switch (irCtx->clangTargetInfo->getLongDoubleWidth()) {
		case 16: {
			floatKind = FloatTypeKind::_16;
			break;
		}
		case 32: {
			floatKind = FloatTypeKind::_32;
			break;
		}
		case 64: {
			floatKind = FloatTypeKind::_64;
			break;
		}
		case 80: {
			floatKind = FloatTypeKind::_80;
			break;
		}
		case 128: {
			floatKind = FloatTypeKind::_128;
			break;
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::FloatType::get(floatKind, irCtx->llctx),
	                         NativeTypeKind::LongDouble);
}

Maybe<bool> NativeType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (subType->can_be_prerun_generic() && first->get_ir_type()->is_same(second->get_ir_type())) {
		return subType->equality_of(irCtx, first, second);
	}
	return None;
}

String NativeType::to_string() const {
	if (nativeKind == NativeTypeKind::ByteString) {
		String res = "bytestring";
		if (subType->as_ptr()->is_non_nullable()) {
			res += "!";
		}
		if (subType->as_ptr()->is_subtype_variable() || subType->as_ptr()->has_address_space()) {
			if (subType->as_ptr()->is_non_nullable()) {
				res += "[";
			} else {
				res += ":[";
			}
			if (subType->as_ptr()->is_subtype_variable()) {
				res += "var";
				if (subType->as_ptr()->has_address_space()) {
					res += ", ";
				}
			}
			if (subType->as_ptr()->has_address_space()) {
				res += subType->as_ptr()->get_address_space().value().to_string();
			}
			res += "]";
		}
		return res;
	} else if (nativeKind == NativeTypeKind::PtrDiff || nativeKind == NativeTypeKind::UPtrDiff) {
		String res = nativeKind == NativeTypeKind::PtrDiff ? "ptrdiff" : "uptrdiff";
		if (addressSpace.has_value()) {
			res += ":[" + addressSpace.value().to_string() + "]";
		}
		return res;
	}
	return NativeType::kind_to_string(nativeKind);
}

} // namespace qat::ir
