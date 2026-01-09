#ifndef QAT_IR_TYPES_INTEGER_HPP
#define QAT_IR_TYPES_INTEGER_HPP

#include "./qat_type.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class IntegerType final : public Type {
  private:
	const u64 bitWidth;
	ir::Ctx*  irCtx;

	static Vec<IntegerType*> allIntegerTypes;

  public:
	IntegerType(u64 _bitWidth, ir::Ctx* irCtx);
	static IntegerType* get(u64 _bits, ir::Ctx* irCtx);

	bool is_bitwidth(u64 width) const { return bitWidth == width; }

	u64 get_bitwidth() const { return bitWidth; }

	bool is_type_sized() const final { return true; }

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	bool has_prerun_default_value() const final { return true; }

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx);
	Maybe<String>    to_prerun_generic_string(ir::PrerunValue* val) const final;
	Maybe<bool>      equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	TypeKind type_kind() const final { return TypeKind::INTEGER; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
