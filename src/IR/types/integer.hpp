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

  public:
	IntegerType(u64 _bitWidth, ir::Ctx* irCtx);
	useit static IntegerType* get(u64 _bits, ir::Ctx* irCtx);

	useit bool is_bitwidth(u64 width) const { return bitWidth == width; }

	useit u64 get_bitwidth() const { return bitWidth; }

	useit bool is_type_sized() const final { return true; }

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	useit bool has_prerun_default_value() const final { return true; }

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx);
	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;
	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	useit TypeKind type_kind() const final { return TypeKind::INTEGER; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
