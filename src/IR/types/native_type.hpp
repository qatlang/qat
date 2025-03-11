#ifndef QAT_IR_TYPES_NATIVE_TYPE_HPP
#define QAT_IR_TYPES_NATIVE_TYPE_HPP

#include "./qat_type.hpp"

namespace qat::ir {

// TODO - Support C arrays
enum class NativeTypeKind {
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

Maybe<NativeTypeKind> native_type_kind_from_string(String const& val);

String native_type_kind_to_string(NativeTypeKind kind);

class NativeType : public Type {
  private:
	ir::Type*      subType;
	NativeTypeKind nativeKind;

  public:
	NativeType(ir::Type* actual, NativeTypeKind c_kind);

	useit NativeTypeKind get_c_type_kind() const;
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

	useit bool is_int() const { return nativeKind == NativeTypeKind::Int; }

	useit bool is_uint() const { return nativeKind == NativeTypeKind::Uint; }

	useit bool is_cbool() const { return nativeKind == NativeTypeKind::Bool; }

	useit bool is_char() const { return nativeKind == NativeTypeKind::Char; }

	useit bool is_char_unsigned() const { return nativeKind == NativeTypeKind::UChar; }

	useit bool is_short() const { return nativeKind == NativeTypeKind::Short; }

	useit bool is_short_unsigned() const { return nativeKind == NativeTypeKind::Short; }

	useit bool is_wide_char() const { return nativeKind == NativeTypeKind::WideChar; }

	useit bool is_wide_char_unsigned() const { return nativeKind == NativeTypeKind::UWideChar; }

	useit bool is_long_int() const { return nativeKind == NativeTypeKind::LongInt; }

	useit bool is_long_int_unsigned() const { return nativeKind == NativeTypeKind::ULongInt; }

	useit bool is_long_long() const { return nativeKind == NativeTypeKind::LongLong; }

	useit bool is_long_long_unsigned() const { return nativeKind == NativeTypeKind::ULongLong; }

	useit bool is_usize() const { return nativeKind == NativeTypeKind::Usize; }

	useit bool is_isize() const { return nativeKind == NativeTypeKind::Isize; }

	useit bool is_cfloat() const { return nativeKind == NativeTypeKind::Float; }

	useit bool is_double() const { return nativeKind == NativeTypeKind::Double; }

	useit bool is_int_max() const { return nativeKind == NativeTypeKind::IntMax; }

	useit bool is_int_max_unsigned() const { return nativeKind == NativeTypeKind::UintMax; }

	useit bool is_c_ptr() const { return nativeKind == NativeTypeKind::Pointer; }

	useit bool is_intptr() const { return nativeKind == NativeTypeKind::IntPtr; }

	useit bool is_intptr_unsigned() const { return nativeKind == NativeTypeKind::UintPtr; }

	useit bool is_ptrdiff() const { return nativeKind == NativeTypeKind::PtrDiff; }

	useit bool is_ptrdiff_unsigned() const { return nativeKind == NativeTypeKind::UPtrDiff; }

	useit bool is_sig_atomic() const { return nativeKind == NativeTypeKind::SigAtomic; }

	useit bool is_process_id() const { return nativeKind == NativeTypeKind::ProcessID; }

	useit bool is_cstring() const { return nativeKind == NativeTypeKind::String; }

	useit bool is_long_double() const { return nativeKind == NativeTypeKind::LongDouble; }

	useit static NativeType* get_from_kind(NativeTypeKind kind, ir::Ctx* irCtx);
	useit static NativeType* get_int(ir::Ctx* irCtx);
	useit static NativeType* get_uint(ir::Ctx* irCtx);
	useit static NativeType* get_bool(ir::Ctx* irCtx);
	useit static NativeType* get_char(ir::Ctx* irCtx);
	useit static NativeType* get_char_unsigned(ir::Ctx* irCtx);
	useit static NativeType* get_short(ir::Ctx* irCtx);
	useit static NativeType* get_short_unsigned(ir::Ctx* irCtx);
	useit static NativeType* get_wide_char(ir::Ctx* irCtx);
	useit static NativeType* get_wide_char_unsigned(ir::Ctx* irCtx);
	useit static NativeType* get_long_int(ir::Ctx* irCtx);
	useit static NativeType* get_long_int_unsigned(ir::Ctx* irCtx);
	useit static NativeType* get_long_long(ir::Ctx* irCtx);
	useit static NativeType* get_long_long_unsigned(ir::Ctx* irCtx);
	useit static NativeType* get_usize(ir::Ctx* irCtx);
	useit static NativeType* get_isize(ir::Ctx* irCtx);
	useit static NativeType* get_float(ir::Ctx* irCtx);
	useit static NativeType* get_double(ir::Ctx* irCtx);
	useit static NativeType* get_intmax(ir::Ctx* irCtx);
	useit static NativeType* get_uintmax(ir::Ctx* irCtx);
	useit static NativeType* get_ptr(bool isSubTypeVariable, ir::Type* subType, ir::Ctx* irCtx);
	useit static NativeType* get_intptr(ir::Ctx* irCtx);
	useit static NativeType* get_uintptr(ir::Ctx* irCtx);
	useit static NativeType* get_ptrdiff(ir::Ctx* irCtx);
	useit static NativeType* get_ptrdiff_unsigned(ir::Ctx* irCtx);
	// TODO - Check if there is more to SigAtomic than just an integer type
	useit static NativeType* get_sigatomic(ir::Ctx* irCtx);
	useit static NativeType* get_processid(ir::Ctx* irCtx);
	useit static NativeType* get_cstr(ir::Ctx* irCtx);

	useit static bool        has_long_double(ir::Ctx* irCtx);
	useit static NativeType* get_long_double(ir::Ctx* irCtx);

	useit bool is_type_sized() const final { return true; }

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	TypeKind type_kind() const final { return TypeKind::NATIVE; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
