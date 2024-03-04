#ifndef QAT_IR_TYPES_OPAQUE_HPP
#define QAT_IR_TYPES_OPAQUE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../generics.hpp"
#include "../meta_info.hpp"
#include "qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ast {
class DefineCoreType;
class DefineMixType;
} // namespace qat::ast

namespace qat::ir {

class Mod;
class Method;

enum class OpaqueSubtypeKind { core, mix, Union, unknown };

class OpaqueType : public Type, public EntityOverview {
  friend class ast::DefineCoreType;
  friend class ast::DefineMixType;
  friend class ir::StructType;

  Identifier               name;
  Vec<GenericParameter*>   generics;
  Maybe<String>            genericID;
  Maybe<OpaqueSubtypeKind> subtypeKind;
  ir::Mod*                 parent;
  ir::ExpandedType*        subTy = nullptr;
  Maybe<usize>             size;
  VisibilityInfo           visibility;
  Maybe<MetaInfo>          metaInfo;

  OpaqueType(Identifier _name, Vec<GenericParameter*> _generics, Maybe<String> _genericID,
             Maybe<OpaqueSubtypeKind> subtypeKind, ir::Mod* _parent, Maybe<usize> _size, VisibilityInfo _visibility,
             llvm::LLVMContext& llctx, Maybe<MetaInfo> metaInfo);

public:
  useit static OpaqueType* get(Identifier name, Vec<GenericParameter*> generics, Maybe<String> genericID,
                               Maybe<OpaqueSubtypeKind> subtypeKind, ir::Mod* parent, Maybe<usize> size,
                               VisibilityInfo visibility, llvm::LLVMContext& llCtx, Maybe<MetaInfo> metaInfo);

  useit String     get_full_name() const;
  useit Identifier get_name() const;
  useit ir::Mod*              get_module() const;
  useit VisibilityInfo const& get_visibility() const;
  useit bool                  is_generic() const;
  useit Maybe<String>     get_generic_id() const;
  useit bool              has_generic_parameter(String const& name) const;
  useit GenericParameter* get_generic_parameter(String const& name) const;

  useit bool  is_subtype_struct() const;
  useit bool  is_subtype_mix() const;
  useit bool  is_subtype_unknown() const;
  useit bool  has_subtype() const;
  void        set_sub_type(ir::ExpandedType* _subTy);
  useit Type* get_subtype() const;
  useit bool  has_deduced_size() const;
  useit usize get_deduced_size() const;

  useit bool is_expanded() const final;
  useit bool can_be_prerun() const final;
  useit bool can_be_prerun_generic() const final;
  useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* preVal) const final;
  useit bool          is_type_sized() const final;
  useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
  useit bool        is_trivially_copyable() const final;
  useit bool        is_trivially_movable() const final;

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

  useit TypeKind type_kind() const final;
  useit String   to_string() const final;
};

} // namespace qat::ir

#endif