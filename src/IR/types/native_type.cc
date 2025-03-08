#include "./native_type.hpp"
#include "../context.hpp"
#include "float.hpp"
#include "integer.hpp"
#include "pointer.hpp"
#include "type_kind.hpp"
#include "unsigned.hpp"

namespace qat::ir {

Maybe<NativeTypeKind> native_type_kind_from_string(String const& val) {
	if (val == "cstring") {
		return NativeTypeKind::String;
	} else if (val == "int") {
		return NativeTypeKind::Int;
	} else if (val == "uint") {
		return NativeTypeKind::Uint;
	} else if (val == "char") {
		return NativeTypeKind::Char;
	} else if (val == "uchar") {
		return NativeTypeKind::UChar;
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
	} else if (val == "cSigAtomic") {
		return NativeTypeKind::SigAtomic;
	} else if (val == "cProcessID") {
		return NativeTypeKind::ProcessID;
	} else if (val == "cBool") {
		return NativeTypeKind::Bool;
	} else if (val == "cPtr") {
		return NativeTypeKind::Pointer;
	}
	return None;
}

String native_type_kind_to_string(NativeTypeKind kind) {
	switch (kind) {
		case NativeTypeKind::Int:
			return "int";
		case NativeTypeKind::Uint:
			return "uint";
		case NativeTypeKind::Char:
			return "char";
		case NativeTypeKind::UChar:
			return "uchar";
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
			return "cSigAtomic";
		case NativeTypeKind::ProcessID:
			return "cProcessID";
		case NativeTypeKind::String:
			return "cStr";
		case NativeTypeKind::Bool:
			return "cBool";
		case NativeTypeKind::Pointer:
			return "cPtr";
	}
}

NativeType::NativeType(ir::Type* actual, NativeTypeKind c_kind) : subType(actual), nativeKind(c_kind) {
	llvmType    = actual->get_llvm_type();
	linkingName = "qat'nativetype:[" + to_string() + "]";
}

NativeTypeKind NativeType::get_c_type_kind() const { return nativeKind; }

ir::Type* NativeType::get_subtype() const { return subType; }

NativeType* NativeType::get_from_kind(NativeTypeKind kind, ir::Ctx* irCtx) {
	switch (kind) {
		case NativeTypeKind::String:
			return get_cstr(irCtx);
		case NativeTypeKind::Bool:
			return get_bool(irCtx);
		case NativeTypeKind::Int:
			return get_int(irCtx);
		case NativeTypeKind::Uint:
			return get_uint(irCtx);
		case NativeTypeKind::Char:
			return get_char(irCtx);
		case NativeTypeKind::UChar:
			return get_char_unsigned(irCtx);
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
			return get_ptrdiff(irCtx);
		case NativeTypeKind::UPtrDiff:
			return get_ptrdiff_unsigned(irCtx);
		case NativeTypeKind::Pointer:
			return nullptr;
		case NativeTypeKind::SigAtomic:
			return get_sigatomic(irCtx);
		case NativeTypeKind::ProcessID:
			return get_processid(irCtx);
		case NativeTypeKind::LongDouble:
			return get_long_double(irCtx);
	}
}

NativeType* NativeType::get_bool(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Bool) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getBoolWidth(), irCtx),
	                         NativeTypeKind::Bool);
}

NativeType* NativeType::get_int(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Int) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::IntegerType::get(irCtx->clangTargetInfo->getIntWidth(), irCtx),
	                         NativeTypeKind::Int);
}

NativeType* NativeType::get_uint(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Uint) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getIntWidth(), irCtx),
	                         NativeTypeKind::Uint);
}

NativeType* NativeType::get_char(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Char) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::IntegerType::get(irCtx->clangTargetInfo->getCharWidth(), irCtx),
	                         NativeTypeKind::Char);
}

NativeType* NativeType::get_char_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::UChar) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getCharWidth(), irCtx),
	                         NativeTypeKind::UChar);
}

NativeType* NativeType::get_short(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Short) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getShortWidth(), irCtx),
	                         NativeTypeKind::Char);
}

NativeType* NativeType::get_short_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::UShort) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getShortWidth(), irCtx),
	                         NativeTypeKind::UChar);
}

NativeType* NativeType::get_wide_char(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::WideChar) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getWCharWidth(), irCtx),
	                         NativeTypeKind::WideChar);
}

NativeType* NativeType::get_wide_char_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::UWideChar) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getWCharWidth(), irCtx),
	                         NativeTypeKind::UWideChar);
}

NativeType* NativeType::get_long_int(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::LongInt) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType), ir::IntegerType::get(irCtx->clangTargetInfo->getLongWidth(), irCtx),
	                         NativeTypeKind::LongInt);
}

NativeType* NativeType::get_long_int_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::ULongInt) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getLongWidth(), irCtx),
	                         NativeTypeKind::ULongInt);
}

NativeType* NativeType::get_long_long(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::LongLong) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getLongLongWidth(), irCtx),
	                         NativeTypeKind::LongLong);
}

NativeType* NativeType::get_long_long_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::ULongLong) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getLongLongWidth(), irCtx),
	                         NativeTypeKind::ULongLong);
}

NativeType* NativeType::get_isize(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Isize) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSignedSizeType()), irCtx),
	    NativeTypeKind::Isize);
}

NativeType* NativeType::get_usize(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Usize) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::UnsignedType::create(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()), irCtx),
	    NativeTypeKind::Usize);
}

NativeType* NativeType::get_float(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto* cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Float) {
				return cTyp;
			}
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
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Double) {
				return cTyp;
			}
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
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::IntMax) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::IntegerType::get(irCtx->clangTargetInfo->getIntMaxTWidth(), irCtx),
	                         NativeTypeKind::IntMax);
}

NativeType* NativeType::get_uintmax(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::UintMax) {
				return cTyp;
			}
		}
	}
	return std::construct_at(OwnNormal(NativeType),
	                         ir::UnsignedType::create(irCtx->clangTargetInfo->getIntMaxTWidth(), irCtx),
	                         NativeTypeKind::UintMax);
}

NativeType* NativeType::get_ptr(bool isSubTypeVariable, ir::Type* subType, ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::Pointer &&
			    (cTyp->subType->as_mark()->is_subtype_variable() == isSubTypeVariable) &&
			    cTyp->subType->as_mark()->get_subtype()->is_same(subType)) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::MarkType::get(isSubTypeVariable, subType, false, MarkOwner::of_anonymous(), false, irCtx),
	    NativeTypeKind::Pointer);
}

NativeType* NativeType::get_intptr(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::IntPtr) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getIntPtrType()), irCtx),
	    NativeTypeKind::IntPtr);
}

NativeType* NativeType::get_uintptr(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::UintPtr) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::UnsignedType::create(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntMaxType()), irCtx),
	    NativeTypeKind::UintPtr);
}

NativeType* NativeType::get_ptrdiff(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::PtrDiff) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(
	                             irCtx->clangTargetInfo->getPtrDiffType(irCtx->get_language_address_space())),
	                         irCtx),
	    NativeTypeKind::PtrDiff);
}

NativeType* NativeType::get_ptrdiff_unsigned(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::UPtrDiff) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::UnsignedType::create(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUnsignedPtrDiffType(
	                                 irCtx->get_language_address_space())),
	                             irCtx),
	    NativeTypeKind::UPtrDiff);
}

NativeType* NativeType::get_sigatomic(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::SigAtomic) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSigAtomicType()), irCtx),
	    NativeTypeKind::SigAtomic);
}

NativeType* NativeType::get_processid(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::ProcessID) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getProcessIDType()), irCtx),
	    NativeTypeKind::ProcessID);
}

NativeType* NativeType::get_cstr(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::String) {
				return cTyp;
			}
		}
	}
	return std::construct_at(
	    OwnNormal(NativeType),
	    ir::MarkType::get(false, ir::IntegerType::get(8, irCtx), false, MarkOwner::of_anonymous(), false, irCtx),
	    NativeTypeKind::String);
}

bool NativeType::has_long_double(ir::Ctx* irCtx) { return irCtx->clangTargetInfo->hasLongDoubleType(); }

NativeType* NativeType::get_long_double(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::NATIVE) {
			auto cTyp = (NativeType*)typ;
			if (cTyp->nativeKind == NativeTypeKind::LongDouble) {
				return cTyp;
			}
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
	return native_type_kind_to_string(nativeKind) +
	       ((nativeKind == NativeTypeKind::Pointer)
	            ? (String(":[") + (subType->as_mark()->is_subtype_variable() ? "var " : "") +
	               subType->as_mark()->get_subtype()->to_string() + "]")
	            : "");
}

} // namespace qat::ir
