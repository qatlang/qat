#ifndef QAT_IR_TYPES_FLOAT_HPP
#define QAT_IR_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

enum class FloatTypeKind { _brain, _16, _32, _64, _80, _128PPC, _128 };

class FloatType : public Type {
  private:
	FloatTypeKind kind;

	FloatType(FloatTypeKind _kind, llvm::LLVMContext& ctx);

  public:
	useit static FloatType* get(FloatTypeKind _kind, llvm::LLVMContext& ctx);

	useit bool is_trivially_copyable() const final { return true; }
	useit bool is_trivially_movable() const final { return true; }

	useit bool			is_type_sized() const final;
	useit FloatTypeKind get_float_kind() const;
	useit TypeKind		type_kind() const final;
	useit String		to_string() const final;

	useit bool can_be_prerun() const final { return true; }
	useit bool can_be_prerun_generic() const final { return true; }
	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;
	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
};

} // namespace qat::ir

#endif
