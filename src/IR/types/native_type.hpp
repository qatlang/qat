#ifndef QAT_IR_TYPES_NATIVE_TYPE_HPP
#define QAT_IR_TYPES_NATIVE_TYPE_HPP

#include "./address_space.hpp"
#include "./qat_type.hpp"

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

class NativeType : public Type {
  private:
	ir::Type*           subType;
	NativeTypeKind      nativeKind;
	Maybe<AddressSpace> addressSpace;

	static Vec<NativeType*> allNativeTypes;

  public:
	NativeType(ir::Type* actual, NativeTypeKind c_kind, Maybe<AddressSpace> addressSpace = None);

	static Maybe<NativeTypeKind> kind_from_string(String const& val);
	static String                kind_to_string(NativeTypeKind kind);

	NativeTypeKind get_c_type_kind() const;
	ir::Type*      get_subtype() const;

	Maybe<AddressSpace> const& get_address_space() const { return addressSpace; }

	bool can_be_prerun() const final { return subType->can_be_prerun(); }

	bool can_be_prerun_generic() const final { return subType->can_be_prerun_generic(); }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* value) const final {
		if (subType->can_be_prerun_generic()) {
			return subType->to_prerun_generic_string(value);
		}
		return None;
	}

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	bool is_native_int() const { return nativeKind == NativeTypeKind::Int; }

	bool is_native_uint() const { return nativeKind == NativeTypeKind::Uint; }

	bool is_native_bool() const { return nativeKind == NativeTypeKind::Bool; }

	bool is_native_byte() const { return nativeKind == NativeTypeKind::Byte; }

	bool is_native_ubyte() const { return nativeKind == NativeTypeKind::UByte; }

	bool is_native_short() const { return nativeKind == NativeTypeKind::Short; }

	bool is_native_ushort() const { return nativeKind == NativeTypeKind::Short; }

	bool is_native_wide_char() const { return nativeKind == NativeTypeKind::WideChar; }

	bool is_native_wide_uchar() const { return nativeKind == NativeTypeKind::UWideChar; }

	bool is_native_long_int() const { return nativeKind == NativeTypeKind::LongInt; }

	bool is_native_long_int_unsigned() const { return nativeKind == NativeTypeKind::ULongInt; }

	bool is_native_long_long() const { return nativeKind == NativeTypeKind::LongLong; }

	bool is_native_long_long_unsigned() const { return nativeKind == NativeTypeKind::ULongLong; }

	bool is_native_usize() const { return nativeKind == NativeTypeKind::Usize; }

	bool is_native_isize() const { return nativeKind == NativeTypeKind::Isize; }

	bool is_native_float() const { return nativeKind == NativeTypeKind::Float; }

	bool is_native_double() const { return nativeKind == NativeTypeKind::Double; }

	bool is_native_int_max() const { return nativeKind == NativeTypeKind::IntMax; }

	bool is_native_uint_max() const { return nativeKind == NativeTypeKind::UintMax; }

	bool is_native_intptr() const { return nativeKind == NativeTypeKind::IntPtr; }

	bool is_native_uintptr() const { return nativeKind == NativeTypeKind::UintPtr; }

	bool is_native_ptrdiff() const { return nativeKind == NativeTypeKind::PtrDiff; }

	bool is_native_uptrdiff() const { return nativeKind == NativeTypeKind::UPtrDiff; }

	bool is_native_sig_atomic() const { return nativeKind == NativeTypeKind::SigAtomic; }

	bool is_native_bytestring() const { return nativeKind == NativeTypeKind::ByteString; }

	bool is_native_long_double() const { return nativeKind == NativeTypeKind::LongDouble; }

	static NativeType* get_from_kind(NativeTypeKind kind, ir::Ctx* irCtx);
	static NativeType* get_int(ir::Ctx* irCtx);
	static NativeType* get_uint(ir::Ctx* irCtx);
	static NativeType* get_bool(ir::Ctx* irCtx);
	static NativeType* get_byte(ir::Ctx* irCtx);
	static NativeType* get_byte_unsigned(ir::Ctx* irCtx);
	static NativeType* get_short(ir::Ctx* irCtx);
	static NativeType* get_short_unsigned(ir::Ctx* irCtx);
	static NativeType* get_wide_char(ir::Ctx* irCtx);
	static NativeType* get_wide_char_unsigned(ir::Ctx* irCtx);
	static NativeType* get_long_int(ir::Ctx* irCtx);
	static NativeType* get_long_int_unsigned(ir::Ctx* irCtx);
	static NativeType* get_long_long(ir::Ctx* irCtx);
	static NativeType* get_long_long_unsigned(ir::Ctx* irCtx);
	static NativeType* get_usize(ir::Ctx* irCtx);
	static NativeType* get_isize(ir::Ctx* irCtx);
	static NativeType* get_float(ir::Ctx* irCtx);
	static NativeType* get_double(ir::Ctx* irCtx);
	static NativeType* get_intmax(ir::Ctx* irCtx);
	static NativeType* get_uintmax(ir::Ctx* irCtx);
	static NativeType* get_intptr(ir::Ctx* irCtx);
	static NativeType* get_uintptr(ir::Ctx* irCtx);
	static NativeType* get_ptrdiff(Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);
	static NativeType* get_ptrdiff_unsigned(Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);
	// TODO - Check if there is more to SigAtomic than just an integer type
	static NativeType* get_sigatomic(ir::Ctx* irCtx);
	static NativeType* get_bytestring(bool isVar, bool isNonNullable, Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);

	static bool        has_long_double(ir::Ctx* irCtx);
	static NativeType* get_long_double(ir::Ctx* irCtx);

	bool is_type_sized() const final { return true; }

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	TypeKind type_kind() const final { return TypeKind::NATIVE; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
