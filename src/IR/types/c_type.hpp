#ifndef QAT_IR_TYPES_C_TYPE_HPP
#define QAT_IR_TYPES_C_TYPE_HPP

#include "qat_type.hpp"

namespace qat::IR {

// TODO - Support C arrays
enum class CTypeKind {
  String,
  Bool,
  Int,
  Uint,
  Char,
  UChar,
  Short,
  UShort,
  WideChar,
  UWideChar,
  LongInt,
  ULongInt,
  LongLong,
  ULongLong,
  Usize,
  Isize,
  Float,
  Double,
  IntMax,
  UintMax,
  IntPtr,
  UintPtr,
  PtrDiff,
  UPtrDiff,
  Pointer,
  SigAtomic,
  ProcessID,
  LongDouble,
};

Maybe<CTypeKind> cTypeKindFromString(String const& val);

String cTypeKindToString(CTypeKind kind);

class CType : public QatType {
private:
  IR::QatType* subType;
  CTypeKind    cTypeKind;

  CType(IR::QatType* actual, CTypeKind c_kind);

public:
  useit CTypeKind get_c_type_kind() const;
  useit IR::QatType* getSubType() const;

  useit inline bool canBePrerun() const final { return subType->canBePrerun(); }
  useit inline bool canBePrerunGeneric() const final { return subType->canBePrerunGeneric(); }
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* value) const final {
    if (subType->canBePrerunGeneric()) {
      return subType->toPrerunGenericString(value);
    }
    return None;
  }
  useit inline Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;

  useit inline bool isInt() const { return cTypeKind == CTypeKind::Int; }
  useit inline bool isUint() const { return cTypeKind == CTypeKind::Uint; }
  useit inline bool isCBool() const { return cTypeKind == CTypeKind::Bool; }
  useit inline bool isChar() const { return cTypeKind == CTypeKind::Char; }
  useit inline bool isCharUnsigned() const { return cTypeKind == CTypeKind::UChar; }
  useit inline bool isShort() const { return cTypeKind == CTypeKind::Short; }
  useit inline bool isShortUnsigned() const { return cTypeKind == CTypeKind::Short; }
  useit inline bool isWideChar() const { return cTypeKind == CTypeKind::WideChar; }
  useit inline bool isWideCharUnsigned() const { return cTypeKind == CTypeKind::UWideChar; }
  useit inline bool isLongInt() const { return cTypeKind == CTypeKind::LongInt; }
  useit inline bool isLongIntUnsigned() const { return cTypeKind == CTypeKind::ULongInt; }
  useit inline bool isLongLong() const { return cTypeKind == CTypeKind::LongLong; }
  useit inline bool isLongLongUnsigned() const { return cTypeKind == CTypeKind::ULongLong; }
  useit inline bool isUsize() const { return cTypeKind == CTypeKind::Usize; }
  useit inline bool isIsize() const { return cTypeKind == CTypeKind::Isize; }
  useit inline bool isCFloat() const { return cTypeKind == CTypeKind::Float; }
  useit inline bool isDouble() const { return cTypeKind == CTypeKind::Double; }
  useit inline bool isIntMax() const { return cTypeKind == CTypeKind::IntMax; }
  useit inline bool isIntMaxUnsigned() const { return cTypeKind == CTypeKind::UintMax; }
  useit inline bool isCPointer() const { return cTypeKind == CTypeKind::Pointer; }
  useit inline bool isIntPtr() const { return cTypeKind == CTypeKind::IntPtr; }
  useit inline bool isIntPtrUnsigned() const { return cTypeKind == CTypeKind::UintPtr; }
  useit inline bool isPtrDiff() const { return cTypeKind == CTypeKind::PtrDiff; }
  useit inline bool isPtrDiffUnsigned() const { return cTypeKind == CTypeKind::UPtrDiff; }
  useit inline bool isSigAtomic() const { return cTypeKind == CTypeKind::SigAtomic; }
  useit inline bool isProcessID() const { return cTypeKind == CTypeKind::ProcessID; }
  useit inline bool isCString() const { return cTypeKind == CTypeKind::String; }
  useit inline bool isLongDouble() const { return cTypeKind == CTypeKind::LongDouble; }

  useit static CType* getFromCTypeKind(CTypeKind kind, IR::Context* ctx);
  useit static CType* getInt(IR::Context* ctx);
  useit static CType* getUInt(IR::Context* ctx);
  useit static CType* getBool(IR::Context* ctx);
  useit static CType* getChar(IR::Context* ctx);
  useit static CType* getCharUnsigned(IR::Context* ctx);
  useit static CType* getShort(IR::Context* ctx);
  useit static CType* getShortUnsigned(IR::Context* ctx);
  useit static CType* getWideChar(IR::Context* ctx);
  useit static CType* getWideCharUnsigned(IR::Context* ctx);
  useit static CType* getLongInt(IR::Context* ctx);
  useit static CType* getLongIntUnsigned(IR::Context* ctx);
  useit static CType* getLongLong(IR::Context* ctx);
  useit static CType* getLongLongUnsigned(IR::Context* ctx);
  useit static CType* getUsize(IR::Context* ctx);
  useit static CType* getIsize(IR::Context* ctx);
  useit static CType* getFloat(IR::Context* ctx);
  useit static CType* getDouble(IR::Context* ctx);
  useit static CType* getIntMax(IR::Context* ctx);
  useit static CType* getUintMax(IR::Context* ctx);
  useit static CType* getPointer(bool isSubTypeVariable, IR::QatType* subType, IR::Context* ctx);
  useit static CType* getIntPtr(IR::Context* ctx);
  useit static CType* getUintPtr(IR::Context* ctx);
  useit static CType* getPtrDiff(IR::Context* ctx);
  useit static CType* getPtrDiffUnsigned(IR::Context* ctx);
  // TODO - Check if there is more to SigAtomic than just an integer type
  useit static CType* getSigAtomic(IR::Context* ctx);
  useit static CType* getProcessID(IR::Context* ctx);
  useit static CType* getCString(IR::Context* ctx);

  useit static bool   hasLongDouble(IR::Context* ctx);
  useit static CType* getLongDouble(IR::Context* ctx);

  useit bool isTypeSized() const final { return true; }
  useit bool isTriviallyCopyable() const final { return true; }
  useit bool isTriviallyMovable() const final { return true; }

  TypeKind typeKind() const final { return TypeKind::cType; }
  String   toString() const final;
};

} // namespace qat::IR

#endif
