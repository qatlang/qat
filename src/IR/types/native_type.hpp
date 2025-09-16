#ifndef QAT_IR_TYPES_NATIVE_TYPE_HPP
#define QAT_IR_TYPES_NATIVE_TYPE_HPP

#include "./qat_type.hpp"
#include "pointer.hpp"

namespace qat::ir {

// TODO - Support C arrays
enum class NativeTypeKind {
	ByteString,
	Bool,
	Int,
	Uint,
	Byte,
	UByte,
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
	SigAtomic,
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

	useit bool is_native_int() const { return nativeKind == NativeTypeKind::Int; }

	useit bool is_native_uint() const { return nativeKind == NativeTypeKind::Uint; }

	useit bool is_native_bool() const { return nativeKind == NativeTypeKind::Bool; }

	useit bool is_native_byte() const { return nativeKind == NativeTypeKind::Byte; }

	useit bool is_native_ubyte() const { return nativeKind == NativeTypeKind::UByte; }

	useit bool is_native_short() const { return nativeKind == NativeTypeKind::Short; }

	useit bool is_native_ushort() const { return nativeKind == NativeTypeKind::Short; }

	useit bool is_native_wide_char() const { return nativeKind == NativeTypeKind::WideChar; }

	useit bool is_native_wide_uchar() const { return nativeKind == NativeTypeKind::UWideChar; }

	useit bool is_native_long_int() const { return nativeKind == NativeTypeKind::LongInt; }

	useit bool is_native_long_int_unsigned() const { return nativeKind == NativeTypeKind::ULongInt; }

	useit bool is_native_long_long() const { return nativeKind == NativeTypeKind::LongLong; }

	useit bool is_native_long_long_unsigned() const { return nativeKind == NativeTypeKind::ULongLong; }

	useit bool is_native_usize() const { return nativeKind == NativeTypeKind::Usize; }

	useit bool is_native_isize() const { return nativeKind == NativeTypeKind::Isize; }

	useit bool is_native_float() const { return nativeKind == NativeTypeKind::Float; }

	useit bool is_native_double() const { return nativeKind == NativeTypeKind::Double; }

	useit bool is_native_int_max() const { return nativeKind == NativeTypeKind::IntMax; }

	useit bool is_native_uint_max() const { return nativeKind == NativeTypeKind::UintMax; }

	useit bool is_native_intptr() const { return nativeKind == NativeTypeKind::IntPtr; }

	useit bool is_native_uintptr() const { return nativeKind == NativeTypeKind::UintPtr; }

	useit bool is_native_ptrdiff() const { return nativeKind == NativeTypeKind::PtrDiff; }

	useit bool is_native_uptrdiff() const { return nativeKind == NativeTypeKind::UPtrDiff; }

	useit bool is_native_sig_atomic() const { return nativeKind == NativeTypeKind::SigAtomic; }

	useit bool is_native_bytestring() const { return nativeKind == NativeTypeKind::ByteString; }

	useit bool is_native_long_double() const { return nativeKind == NativeTypeKind::LongDouble; }

	useit static NativeType* get_from_kind(NativeTypeKind kind, ir::Ctx* irCtx);
	useit static NativeType* get_int(ir::Ctx* irCtx);
	useit static NativeType* get_uint(ir::Ctx* irCtx);
	useit static NativeType* get_bool(ir::Ctx* irCtx);
	useit static NativeType* get_byte(ir::Ctx* irCtx);
	useit static NativeType* get_byte_unsigned(ir::Ctx* irCtx);
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
	useit static NativeType* get_intptr(ir::Ctx* irCtx);
	useit static NativeType* get_uintptr(ir::Ctx* irCtx);
	useit static NativeType* get_ptrdiff(ir::Ctx* irCtx);
	useit static NativeType* get_ptrdiff_unsigned(ir::Ctx* irCtx);
	// TODO - Check if there is more to SigAtomic than just an integer type
	useit static NativeType* get_sigatomic(ir::Ctx* irCtx);
	useit static NativeType* get_bytestring(Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);

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
