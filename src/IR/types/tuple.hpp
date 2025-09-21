#ifndef QAT_IR_TYPES_TUPLE_HPP
#define QAT_IR_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class TupleType : public Type {
  private:
	Vec<Type*>      subTypes;
	Vec<Identifier> names;
	bool            isPacked;

	static Vec<TupleType*> allTupleTypes;

  public:
	TupleType(Vec<Type*> _types, Vec<Identifier> _names, bool _isPacked, llvm::LLVMContext& llctx);
	useit static TupleType* get(Vec<Type*> types, bool isPacked, llvm::LLVMContext& llctx);

	useit static TupleType* create_named(Vec<Type*> types, Vec<Identifier> names, bool isPacked,
	                                     llvm::LLVMContext& llctx);

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

	useit bool is_type_sized() const { return true; }

	useit bool has_simple_copy() const final {
		for (auto* sub : subTypes) {
			if (not sub->has_simple_copy()) {
				return false;
			}
		}
		return true;
	}

	useit bool has_simple_move() const final {
		for (auto* sub : subTypes) {
			if (not sub->has_simple_move()) {
				return false;
			}
		}
		return true;
	}

	useit bool has_named_elements() const { return not names.empty(); }

	useit Vec<Identifier> const& get_element_names() const { return names; }

	useit Vec<Type*> const& get_all_types() const { return subTypes; }

	useit Identifier const& get_name_at(u32 index) const { return names[index]; }

	useit Type* get_type_at(u32 index) const { return subTypes[index]; }

	useit u32 get_element_count() const { return subTypes.size(); }

	useit bool is_packed_tuple() const { return llvm::cast<llvm::StructType>(llvmType)->isPacked(); }

	useit TypeKind type_kind() const final { return TypeKind::TUPLE; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
