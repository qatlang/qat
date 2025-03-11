#ifndef QAT_IR_TYPES_ARRAY_HPP
#define QAT_IR_TYPES_ARRAY_HPP

#include "../../utils/helpers.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class ArrayType : public Type {
  private:
	Type* elementType;
	u64   length;

  public:
	ArrayType(Type* _element_type, u64 _length, llvm::LLVMContext& llctx);
	useit static ArrayType* get(Type* elementType, u64 length, llvm::LLVMContext& llctx);

	useit Type* get_element_type();

	useit u64 get_length() const;

	useit TypeKind type_kind() const final;

	useit String to_string() const final;

	useit bool can_be_prerun() const final { return elementType->can_be_prerun(); }

	useit bool can_be_prerun_generic() const final { return elementType->can_be_prerun_generic(); }

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	useit bool is_type_sized() const final;

	useit bool has_simple_copy() const final { return elementType->has_simple_copy(); }

	useit bool has_simple_move() const final { return elementType->has_simple_move(); }

	useit bool has_prerun_default_value() const final { return elementType->has_prerun_default_value(); }

	useit bool is_copy_constructible() const final;

	useit bool is_move_constructible() const final;

	useit bool is_copy_assignable() const final;

	useit bool is_move_assignable() const final;

	useit bool is_destructible() const final;

	useit ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;
};

} // namespace qat::ir

#endif
