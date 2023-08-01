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
  CTypeKind    get_c_type_kind() const;
  IR::QatType* getSubType() const;

  bool isInt() const;
  bool isUint() const;
  bool isCBool() const;
  bool isChar() const;
  bool isCharUnsigned() const;
  bool isWideChar() const;
  bool isWideCharUnsigned() const;
  bool isLongInt() const;
  bool isLongIntUnsigned() const;
  bool isLongLong() const;
  bool isLongLongUnsigned() const;
  bool isUsize() const;
  bool isIsize() const;
  bool isCFloat() const;
  bool isDouble() const;
  bool isIntMax() const;
  bool isIntMaxUnsigned() const;
  bool isCPointer() const;
  bool isIntPtr() const;
  bool isIntPtrUnsigned() const;
  bool isPtrDiff() const;
  bool isPtrDiffUnsigned() const;
  bool isSigAtomic() const;
  bool isProcessID() const;
  bool isCString() const;
  bool isHalfFloat() const;
  bool isBrainFloat() const;
  bool isFloat128() const;
  bool isLongDouble() const;

  static CType* getInt(IR::Context* ctx);
  static CType* getUInt(IR::Context* ctx);
  static CType* getBool(IR::Context* ctx);
  static CType* getChar(IR::Context* ctx);
  static CType* getCharUnsigned(IR::Context* ctx);
  static CType* getWideChar(IR::Context* ctx);
  static CType* getWideCharUnsigned(IR::Context* ctx);
  static CType* getLongInt(IR::Context* ctx);
  static CType* getLongIntUnsigned(IR::Context* ctx);
  static CType* getLongLong(IR::Context* ctx);
  static CType* getLongLongUnsigned(IR::Context* ctx);
  static CType* getUsize(IR::Context* ctx);
  static CType* getIsize(IR::Context* ctx);
  static CType* getFloat(IR::Context* ctx);
  static CType* getDouble(IR::Context* ctx);
  static CType* getIntMax(IR::Context* ctx);
  static CType* getUintMax(IR::Context* ctx);
  static CType* getPointer(bool isSubTypeVariable, IR::QatType* subType, IR::Context* ctx);
  static CType* getIntPtr(IR::Context* ctx);
  static CType* getUintPtr(IR::Context* ctx);
  static CType* getPtrDiff(IR::Context* ctx);
  static CType* getPtrDiffUnsigned(IR::Context* ctx);
  // TOOD - Check if there is more to SigAtomic than just an integer type
  static CType* getSigAtomic(IR::Context* ctx);
  static CType* getProcessID(IR::Context* ctx);
  static CType* getCString(IR::Context* ctx);

  static bool   hasHalfFloat(IR::Context* ctx);
  static CType* getHalfFloat(IR::Context* ctx);

  static bool   hasBrainFloat(IR::Context* ctx);
  static CType* getBrainFloat(IR::Context* ctx);

  static bool   hasFloat128(IR::Context* ctx);
  static CType* getFloat128(IR::Context* ctx);

  static bool   hasLongDouble(IR::Context* ctx);
  static CType* getLongDouble(IR::Context* ctx);

  TypeKind typeKind() const final;
  String   toString() const final;
};

} // namespace qat::IR

#endif
