#include "c_type.hpp"
#include "../context.hpp"
#include "float.hpp"
#include "integer.hpp"
#include "pointer.hpp"
#include "type_kind.hpp"
#include "unsigned.hpp"

namespace qat::IR {

bool CType::isInt() const { return cTypeKind == CTypeKind::Int; }

bool CType::isUint() const { return cTypeKind == CTypeKind::Uint; }

bool CType::isCBool() const { return cTypeKind == CTypeKind::Bool; }

bool CType::isChar() const { return cTypeKind == CTypeKind::Char; }

bool CType::isCharUnsigned() const { return cTypeKind == CTypeKind::UChar; }

bool CType::isWideChar() const { return cTypeKind == CTypeKind::WideChar; }

bool CType::isWideCharUnsigned() const { return cTypeKind == CTypeKind::UWideChar; }

bool CType::isLongInt() const { return cTypeKind == CTypeKind::LongInt; }

bool CType::isLongIntUnsigned() const { return cTypeKind == CTypeKind::ULongInt; }

bool CType::isLongLong() const { return cTypeKind == CTypeKind::LongLong; }

bool CType::isLongLongUnsigned() const { return cTypeKind == CTypeKind::LongLong; }

bool CType::isUsize() const { return cTypeKind == CTypeKind::Usize; }

bool CType::isIsize() const { return cTypeKind == CTypeKind::Isize; }

bool CType::isCFloat() const { return cTypeKind == CTypeKind::Float; }

bool CType::isDouble() const { return cTypeKind == CTypeKind::Double; }

bool CType::isIntMax() const { return cTypeKind == CTypeKind::IntMax; }

bool CType::isIntMaxUnsigned() const { return cTypeKind == CTypeKind::UintMax; }

bool CType::isCPointer() const { return cTypeKind == CTypeKind::Pointer; }

bool CType::isIntPtr() const { return cTypeKind == CTypeKind::IntPtr; }

bool CType::isIntPtrUnsigned() const { return cTypeKind == CTypeKind::UintPtr; }

bool CType::isPtrDiff() const { return cTypeKind == CTypeKind::PtrDiff; }

bool CType::isPtrDiffUnsigned() const { return cTypeKind == CTypeKind::UPtrDiff; }

bool CType::isSigAtomic() const { return cTypeKind == CTypeKind::SigAtomic; }

bool CType::isProcessID() const { return cTypeKind == CTypeKind::ProcessID; }

bool CType::isCString() const { return cTypeKind == CTypeKind::String; }

bool CType::isHalfFloat() const { return cTypeKind == CTypeKind::HalfFloat; }

bool CType::isBrainFloat() const { return cTypeKind == CTypeKind::BrainFloat; }

bool CType::isFloat128() const { return cTypeKind == CTypeKind::Float128; }

bool CType::isLongDouble() const { return cTypeKind == CTypeKind::LongDouble; }

Maybe<CTypeKind> cTypeKindFromString(String const& val) {
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
  } else if (val == "cFloat128") {
    return CTypeKind::Float128;
  } else if (val == "cFloatHalf") {
    return CTypeKind::HalfFloat;
  } else if (val == "cFloatBrain") {
    return CTypeKind::BrainFloat;
  }
  return None;
}

String cTypeKindToString(CTypeKind kind) {
  switch (kind) {
    case CTypeKind::Int:
      return "int";
    case CTypeKind::Uint:
      return "uint";
    case CTypeKind::Char:
      return "char";
    case CTypeKind::UChar:
      return "uchar";
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
    case CTypeKind::HalfFloat:
      return "cFloatHalf";
    case CTypeKind::BrainFloat:
      return "cFloatBrain";
    case CTypeKind::Float128:
      return "cFloat128";
    case CTypeKind::String:
      return "cStr";
    case CTypeKind::Bool:
      return "cBool";
    case CTypeKind::Pointer:
      return "cPtr";
  }
}

CType::CType(IR::QatType* actual, CTypeKind c_kind) : subType(actual), cTypeKind(c_kind) {
  llvmType = actual->getLLVMType();
}

CTypeKind CType::get_c_type_kind() const { return cTypeKind; }

IR::QatType* CType::getSubType() const { return subType; }

CType* CType::getBool(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Bool) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getBoolWidth(), ctx->llctx), CTypeKind::Bool);
}

CType* CType::getInt(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Int) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getIntWidth(), ctx->llctx), CTypeKind::Int);
}

CType* CType::getUInt(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Uint) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getIntWidth(), ctx->llctx), CTypeKind::Uint);
}

CType* CType::getChar(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Char) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getCharWidth(), ctx->llctx), CTypeKind::Char);
}

CType* CType::getCharUnsigned(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UChar) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getCharWidth(), ctx->llctx), CTypeKind::UChar);
}

CType* CType::getWideChar(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::WideChar) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getWCharWidth(), ctx->llctx), CTypeKind::WideChar);
}

CType* CType::getWideCharUnsigned(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UWideChar) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getWCharWidth(), ctx->llctx), CTypeKind::UWideChar);
}

CType* CType::getLongInt(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::LongInt) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getLongWidth(), ctx->llctx), CTypeKind::LongInt);
}

CType* CType::getLongIntUnsigned(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::ULongInt) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getLongWidth(), ctx->llctx), CTypeKind::ULongInt);
}

CType* CType::getLongLong(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::LongLong) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getLongLongWidth(), ctx->llctx), CTypeKind::LongLong);
}

CType* CType::getLongLongUnsigned(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::ULongLong) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getLongLongWidth(), ctx->llctx), CTypeKind::ULongLong);
}

CType* CType::getIsize(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Isize) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::IntegerType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSignedSizeType()), ctx->llctx),
      CTypeKind::Isize);
}

CType* CType::getUsize(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Usize) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::UnsignedType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSizeType()), ctx->llctx),
      CTypeKind::Usize);
}

CType* CType::getFloat(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto* cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Float) {
        return cTyp;
      }
    }
  }
  FloatTypeKind floatKind = FloatTypeKind::_32;
  switch (ctx->clangTargetInfo->getFloatWidth()) {
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
  return new CType(IR::FloatType::get(floatKind, ctx->llctx), CTypeKind::Float);
}

CType* CType::getDouble(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Double) {
        return cTyp;
      }
    }
  }
  FloatTypeKind floatKind = FloatTypeKind::_64;
  switch (ctx->clangTargetInfo->getDoubleWidth()) {
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
  return new CType(IR::FloatType::get(floatKind, ctx->llctx), CTypeKind::Double);
}

CType* CType::getIntMax(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::IntMax) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getIntMaxTWidth(), ctx->llctx), CTypeKind::IntMax);
}

CType* CType::getUintMax(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UintMax) {
        return cTyp;
      }
    }
  }
  return new CType(IR::UnsignedType::get(ctx->clangTargetInfo->getIntMaxTWidth(), ctx->llctx), CTypeKind::UintMax);
}

CType* CType::getPointer(bool isSubTypeVariable, IR::QatType* subType, IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Pointer &&
          (cTyp->subType->asPointer()->isSubtypeVariable() == isSubTypeVariable) &&
          cTyp->subType->asPointer()->getSubType()->isSame(subType)) {
        return cTyp;
      }
    }
  }
  return new CType(IR::PointerType::get(isSubTypeVariable, subType, PointerOwner::OfAnonymous(), false, ctx->llctx),
                   CTypeKind::Pointer);
}

CType* CType::getIntPtr(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::IntPtr) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::IntegerType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getIntPtrType()), ctx->llctx),
      CTypeKind::IntPtr);
}

CType* CType::getUintPtr(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UintPtr) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::UnsignedType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getUIntMaxType()), ctx->llctx),
      CTypeKind::UintPtr);
}

CType* CType::getPtrDiff(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::PtrDiff) {
        return cTyp;
      }
    }
  }
  return new CType(IR::IntegerType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getPtrDiffType(
                                            ctx->clangTargetInfo->getProgramAddressSpace())),
                                        ctx->llctx),
                   CTypeKind::PtrDiff);
}

CType* CType::getPtrDiffUnsigned(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::UPtrDiff) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::UnsignedType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getUnsignedPtrDiffType(
                                ctx->clangTargetInfo->getProgramAddressSpace())),
                            ctx->llctx),
      CTypeKind::UPtrDiff);
}

CType* CType::getSigAtomic(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::SigAtomic) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::IntegerType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSigAtomicType()), ctx->llctx),
      CTypeKind::SigAtomic);
}

CType* CType::getProcessID(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::ProcessID) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::IntegerType::get(ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getProcessIDType()), ctx->llctx),
      CTypeKind::ProcessID);
}

CType* CType::getCString(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::String) {
        return cTyp;
      }
    }
  }
  return new CType(
      IR::PointerType::get(false, IR::IntegerType::get(8, ctx->llctx), PointerOwner::OfAnonymous(), false, ctx->llctx),
      CTypeKind::String);
}

bool CType::hasHalfFloat(IR::Context* ctx) { return ctx->clangTargetInfo->hasLegalHalfType(); }

CType* CType::getHalfFloat(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::HalfFloat) {
        return cTyp;
      }
    }
  }
  return new CType(IR::FloatType::get(FloatTypeKind::_16, ctx->llctx), CTypeKind::HalfFloat);
}

bool CType::hasBrainFloat(IR::Context* ctx) { return ctx->clangTargetInfo->hasBFloat16Type(); }

CType* CType::getBrainFloat(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::BrainFloat) {
        return cTyp;
      }
    }
  }
  return new CType(IR::FloatType::get(FloatTypeKind::_brain, ctx->llctx), CTypeKind::BrainFloat);
}

bool CType::hasFloat128(IR::Context* ctx) { return ctx->clangTargetInfo->hasFloat128Type(); }

CType* CType::getFloat128(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::Float128) {
        return cTyp;
      }
    }
  }
  return new CType(IR::FloatType::get(FloatTypeKind::_128, ctx->llctx), CTypeKind::Float128);
}

bool CType::hasLongDouble(IR::Context* ctx) { return ctx->clangTargetInfo->hasLongDoubleType(); }

CType* CType::getLongDouble(IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cType) {
      auto cTyp = (CType*)typ;
      if (cTyp->cTypeKind == CTypeKind::LongDouble) {
        return cTyp;
      }
    }
  }
  FloatTypeKind floatKind = FloatTypeKind::_80;
  switch (ctx->clangTargetInfo->getLongDoubleWidth()) {
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
  return new CType(IR::FloatType::get(floatKind, ctx->llctx), CTypeKind::LongDouble);
}

TypeKind CType::typeKind() const { return TypeKind::cType; }

String CType::toString() const {
  return cTypeKindToString(cTypeKind) +
         ((cTypeKind == CTypeKind::Pointer)
              ? (String("'[") + (subType->asPointer()->isSubtypeVariable() ? "var " : "") +
                 subType->asPointer()->getSubType()->toString() + "]")
              : "");
}

} // namespace qat::IR
