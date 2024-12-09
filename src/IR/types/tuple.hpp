#ifndef QAT_IR_TYPES_TUPLE_HPP
#define QAT_IR_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

#include <vector>

namespace qat::ir {

class TupleType : public Type {
  private:
	Vec<Type*> subTypes;
	bool       isPacked;

  public:
	TupleType(Vec<Type*> _types, bool _isPacked, llvm::LLVMContext& llctx);
	useit static TupleType* get(Vec<Type*> types, bool isPacked, llvm::LLVMContext& llctx);

	useit bool is_copy_constructible() const final;
	useit bool is_copy_assignable() const final;
	useit bool is_move_constructible() const final;
	useit bool is_move_assignable() const final;
	useit bool is_destructible() const final;

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	useit bool is_type_sized() const;
	useit bool is_trivially_copyable() const final {
		for (auto* sub : subTypes) {
			if (!sub->is_trivially_copyable()) {
				return false;
			}
		}
		return true;
	}
	useit bool is_trivially_movable() const final {
		for (auto* sub : subTypes) {
			if (!sub->is_trivially_movable()) {
				return false;
			}
		}
		return true;
	}

	useit Vec<Type*> getSubTypes() const;
	useit Type*      getSubtypeAt(u32 index);
	useit u32        getSubTypeCount() const;
	useit bool       isPackedTuple() const;
	useit TypeKind   type_kind() const final;
	useit String     to_string() const final;
};

} // namespace qat::ir

#endif
