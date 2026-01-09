#ifndef QAT_IR_TYPES_UNSIGNED_HPP
#define QAT_IR_TYPES_UNSIGNED_HPP

#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

// Unsigned integer datatype in the language
class UnsignedType : public Type {
  private:
	u64  bitWidth;
	bool isTypeBool;

	ir::Ctx* irCtx;

	static Vec<UnsignedType*> allUnsignedTypes;

  public:
	UnsignedType(u64 _bitWidth, ir::Ctx* irCtx, bool _isBool);

	static UnsignedType* create(u64 bits, ir::Ctx* llctx);

	static UnsignedType* create_bool(ir::Ctx* llctx);

	u64 get_bitwidth() const { return bitWidth; }

	bool is_bitwidth(u64 width) const { return bitWidth == width; }

	bool is_this_bool_type() const { return isTypeBool; }

	TypeKind type_kind() const final { return TypeKind::UNSIGNED_INTEGER; }

	String to_string() const final { return isTypeBool ? "bool" : ("u" + std::to_string(bitWidth)); }

	bool is_type_sized() const final { return true; }

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	bool has_prerun_default_value() const final { return true; }

	ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx);

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
};

} // namespace qat::ir

#endif
