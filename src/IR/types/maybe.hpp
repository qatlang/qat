#ifndef QAT_IR_MAYBE_HPP
#define QAT_IR_MAYBE_HPP

#include "qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

class MaybeType : public Type {
private:
  Type* subTy;
  bool  isPacked;

  MaybeType(Type* subTy, bool isPacked, ir::Ctx* irCtx);

public:
  useit static MaybeType* get(Type* subTy, bool isPacked, ir::Ctx* irCtx);

  useit bool  has_sized_sub_type(ir::Ctx* irCtx) const;
  useit Type* get_subtype() const;
  useit bool  is_type_sized() const final;
  useit bool  is_trivially_copyable() const final { return subTy->is_trivially_copyable(); }
  useit bool  is_trivially_movable() const final { return subTy->is_trivially_movable(); }
  useit bool  is_type_packed() const;

  useit bool is_copy_constructible() const final;
  void       copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  useit bool is_move_constructible() const final;
  void       move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  useit bool is_copy_assignable() const final;
  void       copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  useit bool is_move_assignable() const final;
  void       move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  useit bool is_destructible() const final;
  void       destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

  useit inline bool can_be_prerun() const final { return subTy->is_type_sized() && subTy->can_be_prerun(); }
  useit inline bool can_be_prerun_generic() const final {
    return subTy->is_type_sized() && subTy->can_be_prerun_generic();
  }
  useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* value) const final;
  useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

  useit String   to_string() const final;
  useit TypeKind type_kind() const final { return TypeKind::maybe; }
};

} // namespace qat::ir

#endif