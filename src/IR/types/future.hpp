#ifndef QAT_IR_FUTURE_HPP
#define QAT_IR_FUTURE_HPP

#include "../context.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ir {

class FutureType final : public Type {
  private:
	Type* subTy;
	bool  isPacked;

	static Vec<FutureType*> allFutureTypes;

  public:
	FutureType(Type* subType, bool isPacked, ir::Ctx* irCtx);
	static FutureType* get(Type* subType, bool isPacked, ir::Ctx* irCtx);

	Type*    get_subtype() const;
	String   to_string() const final;
	TypeKind type_kind() const final;
	bool     is_type_sized() const final;
	bool     is_type_packed() const;

	bool is_copy_constructible() const final;
	bool is_copy_assignable() const final;

	bool is_move_constructible() const final { return false; }

	bool is_move_assignable() const final { return false; }

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	bool is_destructible() const final;
	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;
};

} // namespace qat::ir

#endif
