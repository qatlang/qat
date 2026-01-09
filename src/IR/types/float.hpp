#ifndef QAT_IR_TYPES_FLOAT_HPP
#define QAT_IR_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

enum class FloatTypeKind { _brain, _16, _32, _64, _80, _128PPC, _128 };

class FloatType : public Type {
  private:
	FloatTypeKind kind;

	static Vec<FloatType*> allFloatTypes;

  public:
	FloatType(FloatTypeKind _kind, llvm::LLVMContext& ctx);

	static FloatType* get(FloatTypeKind _kind, llvm::LLVMContext& ctx);

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	bool is_type_sized() const final;

	FloatTypeKind get_float_kind() const;

	TypeKind type_kind() const final;

	String to_string() const final;

	bool has_prerun_default_value() const final { return true; }

	PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
};

} // namespace qat::ir

#endif
