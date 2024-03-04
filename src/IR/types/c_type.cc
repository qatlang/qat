#include "c_type.hpp"
#include "../context.hpp"
#include "float.hpp"
#include "integer.hpp"
#include "pointer.hpp"
#include "type_kind.hpp"
#include "unsigned.hpp"

namespace qat::ir {

Maybe<CTypeKind> ctype_kind_from_string(String const& val) {
  if (val == "cStr") {
    return CTypeKind::String;
  } else if (val == "int") {
    return CTypeKind::Int;
  } else if (val == "uint") {
    return CTypeKind::Uint;
  } else if (val == "char") {
    return CTypeKind::Char;
  } else if (val == "uchar") {
    return CTypeKind::UChar;
  } else if (val == "shortint") {
    return CTypeKind::Short;
  } else if (val == "ushortint") {
    return CTypeKind::UShort;
  } else if (val == "widechar") {
    return CTypeKind::WideChar;
  } else if (val == "uwidechar") {
    return CTypeKind::UWideChar;
  } else if (val == "longint") {
    return CTypeKind::LongInt;
  } else if (val == "ulongint") {
    return CTypeKind::ULongInt;
  } else if (val == "longlong") {
    return CTypeKind::LongLong;
  } else if (val == "ulonglong") {
    return CTypeKind::ULongLong;
  } else if (val == "usize") {
    return CTypeKind::Usize;
  } else if (val == "isize") {
    return CTypeKind::Isize;
  } else if (val == "float") {
    return CTypeKind::Float;
  } else if (val == "double") {
    return CTypeKind::Double;
  } else if (val == "longdouble") {
    return CTypeKind::LongDouble;
  } else if (val == "intmax") {
    return CTypeKind::IntMax;
  } else if (val == "uintmax") {
    return CTypeKind::UintMax;
  } else if (val == "intptr") {
    return CTypeKind::IntPtr;
  } else if (val == "uintptr") {
    return CTypeKind::UintPtr;
  } else if (val == "ptrdiff") {
    return CTypeKind::PtrDiff;
  } else if (val == "uptrdiff") {
    return CTypeKind::UPtrDiff;
  } else if (val == "cSigAtomic") {
    return CTypeKind::SigAtomic;
  } else if (val == "cProcessID") {
    return CTypeKind::ProcessID;
  } else if (val == "cBool") {
    return CTypeKind::Bool;
  } else if (val == "cPtr") {
    return CTypeKind::Pointer;
  }
  return None;
}

String ctype_kind_to_string(CTypeKind kind) {
  switch (kind) {
    case CTypeKind::Int:
      return "int";
    case CTypeKind::Uint:
      return "uint";
    case CTypeKind::Char:
      return "char";
    case CTypeKind::UChar:
      return "uchar";
    case CTypeKind::Short:
      return "shortint";
    case CTypeKind::UShort:
      return "ushortint";
    case CTypeKind::WideChar:
      return "widechar";
    case CTypeKind::UWideChar:
      return "uwidechar";
    case CTypeKind::LongInt:
      return "longint";
    case CTypeKind::ULongInt:
      return "ulongint";
    case CTypeKind::LongLong:
      return "longlong";
    case CTypeKind::ULongLong:
      return "ulonglong";
    case CTypeKind::Usize:
      return "usize";
    case CTypeKind::Isize:
      return "isize";
    case CTypeKind::Float:
      return "float";
    case CTypeKind::Double:
      return "double";
    case CTypeKind::LongDouble:
      return "longdouble";
    case CTypeKind::IntMax:
      return "intmax";
    case CTypeKind::UintMax:
      return "uintmax";
    case CTypeKind::IntPtr:
      return "intptr";
    case CTypeKind::UintPtr:
      return "uintptr";
    case CTypeKind::PtrDiff:
      return "ptrdiff";
    case CTypeKind::UPtrDiff:
      return "uptrdiff";
    case CTypeKind::SigAtomic:
      return "cSigAtomic";
    case CTypeKind::ProcessID:
      return "cProcessID";
    case CTypeKind::String:
      return "cStr";
    case CTypeKind::Bool:
      return "cBool";
    case CTypeKind::Pointer:
      return "cPtr";
  }
}

CType::CType(ir::Type* actual, CTypeKind c_kind) : subType(actual), cTypeKind(c_kind) {
  llvmType    = actual->get_llvm_type();
  linkingName = "qat'ctype:[" + to_string() + "]";
}

CTypeKind CType::get_c_type_kind() const { return cTypeKind; }

ir::Type* CType::get_subtype() const { return subType; }

CType* CType::get_from_ctype_kind(CTypeKind kind, ir::Ctx* irCtx) {
  switch (kind) {
    case CTypeKind::String:
      return get_cstr(irCtx);
    case CTypeKind::Bool:
      return get_bool(irCtx);
    case CTypeKind::Int:
      return get_int(irCtx);
    case CTypeKind::Uint:
      return get_uint(irCtx);
    case CTypeKind::Char:
      return get_char(irCtx);
    case CTypeKind::UChar:
      return get_char_unsigned(irCtx);
    case CTypeKind::Short:
      return get_short(irCtx);
    case CTypeKind::UShort:
      return get_short_unsigned(irCtx);
    case CTypeKind::WideChar:
      return get_wide_char(irCtx);
    case CTypeKind::UWideChar:
      return get_wide_char_unsigned(irCtx);
    case CTypeKind::LongInt:
      return get_long_int(irCtx);
    case CTypeKind::ULongInt:
      return get_long_int_unsigned(irCtx);
    case CTypeKind::LongLong:
      return get_long_long(irCtx);
    case CTypeKind::ULongLong:
      return get_long_long_unsigned(irCtx);
    case CTypeKind::Usize:
      return get_usize(irCtx);
    case CTypeKind::Isize:
      return get_isize(irCtx);
    case CTypeKind::Float:
      return get_float(irCtx);
    case CTypeKind::Double:
      return get_double(irCtx);
    case CTypeKind::IntMax:
      return get_intmax(irCtx);
    case CTypeKind::UintMax:
      return get_uintmax(irCtx);
    case CTypeKind::IntPtr:
      return get_intptr(irCtx);
    case CTypeKind::UintPtr:
      return get_uintptr(irCtx);
    case CTypeKind::PtrDiff:
      return get_ptrdiff(irCtx);
    case CTypeKind::UPtrDiff:
      return get_ptrdiff_unsigned(irCtx);
    case CTypeKind::Pointer:
      return nullptr;
    case CTypeKind::SigAtomic:
      return get_sigatomic(irCtx);
    case CTypeKind::ProcessID:
      return get_processid(irCtx);
    case CTypeKind::LongDouble:
      return get_long_double(irCtx);
  }
}

CType* CType::get_bool(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Bool) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getBoolWidth(), irCtx), CTypeKind::Bool);
}

CType* CType::get_int(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Int) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getIntWidth(), irCtx), CTypeKind::Int);
}

CType* CType::get_uint(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Uint) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getIntWidth(), irCtx), CTypeKind::Uint);
}

CType* CType::get_char(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Char) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getCharWidth(), irCtx), CTypeKind::Char);
}

CType* CType::get_char_unsigned(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UChar) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getCharWidth(), irCtx), CTypeKind::UChar);
}

CType* CType::get_short(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Short) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getShortWidth(), irCtx), CTypeKind::Char);
}

CType* CType::get_short_unsigned(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UShort) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getShortWidth(), irCtx), CTypeKind::UChar);
}

CType* CType::get_wide_char(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::WideChar) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getWCharWidth(), irCtx), CTypeKind::WideChar);
}

CType* CType::get_wide_char_unsigned(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UWideChar) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getWCharWidth(), irCtx), CTypeKind::UWideChar);
}

CType* CType::get_long_int(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::LongInt) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getLongWidth(), irCtx), CTypeKind::LongInt);
}

CType* CType::get_long_int_unsigned(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::ULongInt) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getLongWidth(), irCtx), CTypeKind::ULongInt);
}

CType* CType::get_long_long(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::LongLong) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getLongLongWidth(), irCtx), CTypeKind::LongLong);
}

CType* CType::get_long_long_unsigned(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::ULongLong) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getLongLongWidth(), irCtx), CTypeKind::ULongLong);
}

CType* CType::get_isize(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Isize) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSignedSizeType()), irCtx),
      CTypeKind::Isize);
}

CType* CType::get_usize(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Usize) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::UnsignedType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()), irCtx),
      CTypeKind::Usize);
}

CType* CType::get_float(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Float) {
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
  return new CType(ir::FloatType::get(floatKind, irCtx->llctx), CTypeKind::Float);
}

CType* CType::get_double(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Double) {
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
  return new CType(ir::FloatType::get(floatKind, irCtx->llctx), CTypeKind::Double);
}

CType* CType::get_intmax(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::IntMax) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getIntMaxTWidth(), irCtx), CTypeKind::IntMax);
}

CType* CType::get_uintmax(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UintMax) {
        return cTyp;
      }
    }
  }
  return new CType(ir::UnsignedType::get(irCtx->clangTargetInfo->getIntMaxTWidth(), irCtx), CTypeKind::UintMax);
}

CType* CType::get_ptr(bool isSubTypeVariable, ir::Type* subType, ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Pointer &&
          (cTyp->subType->as_pointer()->isSubtypeVariable() == isSubTypeVariable) &&
          cTyp->subType->as_pointer()->get_subtype()->is_same(subType)) {
        return cTyp;
      }
    }
  }
  return new CType(ir::PointerType::get(isSubTypeVariable, subType, false, PointerOwner::OfAnonymous(), false, irCtx),
                   CTypeKind::Pointer);
}

CType* CType::get_intptr(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::IntPtr) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getIntPtrType()), irCtx),
      CTypeKind::IntPtr);
}

CType* CType::get_uintptr(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UintPtr) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::UnsignedType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntMaxType()), irCtx),
      CTypeKind::UintPtr);
}

CType* CType::get_ptrdiff(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::PtrDiff) {
        return cTyp;
      }
    }
  }
  return new CType(ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getPtrDiffType(
                                            irCtx->get_language_address_space())),
                                        irCtx),
                   CTypeKind::PtrDiff);
}

CType* CType::get_ptrdiff_unsigned(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UPtrDiff) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::UnsignedType::get(irCtx->clangTargetInfo->getTypeWidth(
                                irCtx->clangTargetInfo->getUnsignedPtrDiffType(irCtx->get_language_address_space())),
                            irCtx),
      CTypeKind::UPtrDiff);
}

CType* CType::get_sigatomic(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::SigAtomic) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSigAtomicType()), irCtx),
      CTypeKind::SigAtomic);
}

CType* CType::get_processid(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::ProcessID) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::IntegerType::get(irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getProcessIDType()), irCtx),
      CTypeKind::ProcessID);
}

CType* CType::get_cstr(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::String) {
        return cTyp;
      }
    }
  }
  return new CType(
      ir::PointerType::get(false, ir::IntegerType::get(8, irCtx), false, PointerOwner::OfAnonymous(), false, irCtx),
      CTypeKind::String);
}

bool CType::has_long_double(ir::Ctx* irCtx) { return irCtx->clangTargetInfo->hasLongDoubleType(); }

CType* CType::get_long_double(ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::LongDouble) {
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
  return new CType(ir::FloatType::get(floatKind, irCtx->llctx), CTypeKind::LongDouble);
}

Maybe<bool> CType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
  if (subType->can_be_prerun_generic() && first->get_ir_type()->is_same(second->get_ir_type())) {
    return subType->equality_of(irCtx, first, second);
  }
  return None;
}

String CType::to_string() const {
  return ctype_kind_to_string(cTypeKind) +
         ((cTypeKind == CTypeKind::Pointer)
              ? (String(":[") + (subType->as_pointer()->isSubtypeVariable() ? "var " : "") +
                 subType->as_pointer()->get_subtype()->to_string() + "]")
              : "");
}

} // namespace qat::ir
