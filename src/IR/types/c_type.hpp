#ifndef QAT_IR_TYPES_C_TYPE_HPP
#define QAT_IR_TYPES_C_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ir {

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

Maybe<CTypeKind> ctype_kind_from_string(String const& val);

String ctype_kind_to_string(CTypeKind kind);

class CType : public Type {
private:
  ir::Type* subType;
  CTypeKind cTypeKind;

  CType(ir::Type* actual, CTypeKind c_kind);

public:
  useit CTypeKind get_c_type_kind() const;
  useit ir::Type* get_subtype() const;

  useit bool can_be_prerun() const final { return subType->can_be_prerun(); }
  useit bool can_be_prerun_generic() const final { return subType->can_be_prerun_generic(); }
  useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* value) const final {
    if (subType->can_be_prerun_generic()) {
      return subType->to_prerun_generic_string(value);
    }
    return None;
  }
  useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

  useit bool is_int() const { return cTypeKind == CTypeKind::Int; }
  useit bool is_uint() const { return cTypeKind == CTypeKind::Uint; }
  useit bool is_cbool() const { return cTypeKind == CTypeKind::Bool; }
  useit bool is_char() const { return cTypeKind == CTypeKind::Char; }
  useit bool is_char_unsigned() const { return cTypeKind == CTypeKind::UChar; }
  useit bool is_short() const { return cTypeKind == CTypeKind::Short; }
  useit bool is_short_unsigned() const { return cTypeKind == CTypeKind::Short; }
  useit bool is_wide_char() const { return cTypeKind == CTypeKind::WideChar; }
  useit bool is_wide_char_unsigned() const { return cTypeKind == CTypeKind::UWideChar; }
  useit bool is_long_int() const { return cTypeKind == CTypeKind::LongInt; }
  useit bool is_long_int_unsigned() const { return cTypeKind == CTypeKind::ULongInt; }
  useit bool is_long_long() const { return cTypeKind == CTypeKind::LongLong; }
  useit bool is_long_long_unsigned() const { return cTypeKind == CTypeKind::ULongLong; }
  useit bool is_usize() const { return cTypeKind == CTypeKind::Usize; }
  useit bool is_isize() const { return cTypeKind == CTypeKind::Isize; }
  useit bool is_cfloat() const { return cTypeKind == CTypeKind::Float; }
  useit bool is_double() const { return cTypeKind == CTypeKind::Double; }
  useit bool is_int_max() const { return cTypeKind == CTypeKind::IntMax; }
  useit bool is_int_max_unsigned() const { return cTypeKind == CTypeKind::UintMax; }
  useit bool is_c_ptr() const { return cTypeKind == CTypeKind::Pointer; }
  useit bool is_intptr() const { return cTypeKind == CTypeKind::IntPtr; }
  useit bool is_intptr_unsigned() const { return cTypeKind == CTypeKind::UintPtr; }
  useit bool is_ptrdiff() const { return cTypeKind == CTypeKind::PtrDiff; }
  useit bool is_ptrdiff_unsigned() const { return cTypeKind == CTypeKind::UPtrDiff; }
  useit bool is_sig_atomic() const { return cTypeKind == CTypeKind::SigAtomic; }
  useit bool is_process_id() const { return cTypeKind == CTypeKind::ProcessID; }
  useit bool is_cstring() const { return cTypeKind == CTypeKind::String; }
  useit bool is_long_double() const { return cTypeKind == CTypeKind::LongDouble; }

  useit static CType* get_from_ctype_kind(CTypeKind kind, ir::Ctx* irCtx);
  useit static CType* get_int(ir::Ctx* irCtx);
  useit static CType* get_uint(ir::Ctx* irCtx);
  useit static CType* get_bool(ir::Ctx* irCtx);
  useit static CType* get_char(ir::Ctx* irCtx);
  useit static CType* get_char_unsigned(ir::Ctx* irCtx);
  useit static CType* get_short(ir::Ctx* irCtx);
  useit static CType* get_short_unsigned(ir::Ctx* irCtx);
  useit static CType* get_wide_char(ir::Ctx* irCtx);
  useit static CType* get_wide_char_unsigned(ir::Ctx* irCtx);
  useit static CType* get_long_int(ir::Ctx* irCtx);
  useit static CType* get_long_int_unsigned(ir::Ctx* irCtx);
  useit static CType* get_long_long(ir::Ctx* irCtx);
  useit static CType* get_long_long_unsigned(ir::Ctx* irCtx);
  useit static CType* get_usize(ir::Ctx* irCtx);
  useit static CType* get_isize(ir::Ctx* irCtx);
  useit static CType* get_float(ir::Ctx* irCtx);
  useit static CType* get_double(ir::Ctx* irCtx);
  useit static CType* get_intmax(ir::Ctx* irCtx);
  useit static CType* get_uintmax(ir::Ctx* irCtx);
  useit static CType* get_ptr(bool isSubTypeVariable, ir::Type* subType, ir::Ctx* irCtx);
  useit static CType* get_intptr(ir::Ctx* irCtx);
  useit static CType* get_uintptr(ir::Ctx* irCtx);
  useit static CType* get_ptrdiff(ir::Ctx* irCtx);
  useit static CType* get_ptrdiff_unsigned(ir::Ctx* irCtx);
  // TODO - Check if there is more to SigAtomic than just an integer type
  useit static CType* get_sigatomic(ir::Ctx* irCtx);
  useit static CType* get_processid(ir::Ctx* irCtx);
  useit static CType* get_cstr(ir::Ctx* irCtx);

  useit static bool   has_long_double(ir::Ctx* irCtx);
  useit static CType* get_long_double(ir::Ctx* irCtx);

  useit bool is_type_sized() const final { return true; }
  useit bool is_trivially_copyable() const final { return true; }
  useit bool is_trivially_movable() const final { return true; }

  TypeKind type_kind() const final { return TypeKind::cType; }
  String   to_string() const final;
};

} // namespace qat::ir

#endif
