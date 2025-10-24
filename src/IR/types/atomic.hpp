#ifndef QAT_IR_TYPES_ATOMIC_HPP
#define QAT_IR_TYPES_ATOMIC_HPP

#include "./qat_type.hpp"
#include "llvm/Support/AtomicOrdering.h"

namespace qat::ir {

enum class AtomicOrdering : u8 {
	UNORDERED,
	RELAXED,
	ACQUIRE,
	RELEASE,
	ACQUIRE_AND_RELEASE,
	SEQUENTIALLY_CONSISTENT,
};

class AtomicType final : public Type {
	Type*                 subType;
	Maybe<AtomicOrdering> ordering;

	useit llvm::AtomicOrdering get_llvm_ordering(ir::Ctx* irCtx) const;

  public:
	AtomicType(Type* _subType, Maybe<AtomicOrdering> _ordering) : subType(_subType), ordering(_ordering) {}

	useit Type* get_subtype() const { return subType; }

	useit Maybe<AtomicOrdering> get_ordering() const { return ordering; }

	useit static String ordering_to_string(AtomicOrdering order) {
		switch (order) {
			case AtomicOrdering::UNORDERED:
				return "meta:AtomicOrdering::unordered";
			case AtomicOrdering::RELAXED:
				return "meta:AtomicOrdering::relaxed";
			case AtomicOrdering::ACQUIRE:
				return "meta:AtomicOrdering::acquire";
			case AtomicOrdering::RELEASE:
				return "meta:AtomicOrdering::release";
			case AtomicOrdering::ACQUIRE_AND_RELEASE:
				return "meta:AtomicOrdering::acquire_and_release";
			case AtomicOrdering::SEQUENTIALLY_CONSISTENT:
				return "meta:AtomicOrdering::sequentially_consistent";
		}
	}

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return false; }

	useit bool has_prerun_default_value() const final { return true; }

	useit bool is_type_sized() const final { return true; }

	useit bool is_default_constructible() const final { return subType->has_prerun_default_value(); }

	useit bool is_copy_constructible() const final { return true; }

	useit bool is_copy_assignable() const final { return true; }

	useit bool is_move_constructible() const final { return true; }

	useit bool is_move_assignable() const final { return true; }

	useit bool is_destructible() const final { return true; }

	useit bool has_simple_copy() const final { return false; }

	useit bool has_simple_move() const final { return false; }

	void default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	useit String to_string() const final {
		return "atomic:[" + subType->to_string() +
		       (ordering.has_value() ? (", " + ordering_to_string(ordering.value())) : "") + "]";
	}

	useit TypeKind type_kind() const final { return TypeKind::ATOMIC; }
};

} // namespace qat::ir

#endif
