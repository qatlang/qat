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
  HalfFloat,
  BrainFloat,
  Float128,
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

  useit bool isInt() const;
  useit bool isUint() const;
  useit bool isCBool() const;
  useit bool isChar() const;
  useit bool isCharUnsigned() const;
  useit bool isWideChar() const;
  useit bool isWideCharUnsigned() const;
  useit bool isLongInt() const;
  useit bool isLongIntUnsigned() const;
  useit bool isLongLong() const;
  useit bool isLongLongUnsigned() const;
  useit bool isUsize() const;
  useit bool isIsize() const;
  useit bool isCFloat() const;
  useit bool isDouble() const;
  useit bool isIntMax() const;
  useit bool isIntMaxUnsigned() const;
  useit bool isCPointer() const;
  useit bool isIntPtr() const;
  useit bool isIntPtrUnsigned() const;
  useit bool isPtrDiff() const;
  useit bool isPtrDiffUnsigned() const;
  useit bool isSigAtomic() const;
  useit bool isProcessID() const;
  useit bool isCString() const;
  useit bool isHalfFloat() const;
  useit bool isBrainFloat() const;
  useit bool isFloat128() const;
  useit bool isLongDouble() const;

  useit static CType* getFromCTypeKind(CTypeKind kind, IR::Context* ctx);
  useit static CType* getInt(IR::Context* ctx);
  useit static CType* getUInt(IR::Context* ctx);
  useit static CType* getBool(IR::Context* ctx);
  useit static CType* getChar(IR::Context* ctx);
  useit static CType* getCharUnsigned(IR::Context* ctx);
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
  // TOOD - Check if there is more to SigAtomic than just an integer type
  useit static CType* getSigAtomic(IR::Context* ctx);
  useit static CType* getProcessID(IR::Context* ctx);
  useit static CType* getCString(IR::Context* ctx);

  useit static bool   hasHalfFloat(IR::Context* ctx);
  useit static CType* getHalfFloat(IR::Context* ctx);

  useit static bool   hasBrainFloat(IR::Context* ctx);
  useit static CType* getBrainFloat(IR::Context* ctx);

  useit static bool   hasFloat128(IR::Context* ctx);
  useit static CType* getFloat128(IR::Context* ctx);

  useit static bool   hasLongDouble(IR::Context* ctx);
  useit static CType* getLongDouble(IR::Context* ctx);

  useit bool isTypeSized() const final;
  useit bool isTriviallyCopyable() const final {
    switch (cTypeKind) {
      case CTypeKind::String:
      case CTypeKind::Pointer:
        return false;
      default:
        return true;
    }
  }
  useit bool isTriviallyMovable() const final {
    switch (cTypeKind) {
      case CTypeKind::String:
      case CTypeKind::Pointer:
        return false;
      default:
        return true;
    }
  }

  TypeKind typeKind() const final;
  String   toString() const final;
};

} // namespace qat::IR

#endif
