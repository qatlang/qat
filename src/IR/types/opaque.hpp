#ifndef QAT_IR_TYPES_OPAQUE_HPP
#define QAT_IR_TYPES_OPAQUE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/mentionable.hpp"
#include "../../utils/visibility.hpp"
#include "../generics.hpp"
#include "../meta_info.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ast {
class DefineStructType;
class DefineMixType;
} // namespace qat::ast

namespace qat::ir {

class Mod;
class Method;

enum class OpaqueSubtypeKind { STRUCT, MIX, TOGGLE, unknown };

class OpaqueType : public Type, public Mentionable {
	friend class ast::DefineStructType;
	friend class ast::DefineMixType;
	friend class ir::StructType;

	Identifier               name;
	Vec<GenericArgument*>    generics;
	Maybe<u64>               genericID;
	Maybe<OpaqueSubtypeKind> subtypeKind;
	ir::Mod*                 parent;
	ir::ExpandedType*        subTy = nullptr;
	Maybe<usize>             size;
	VisibilityInfo           visibility;
	Maybe<MetaInfo>          metaInfo;

  public:
	OpaqueType(Identifier _name, Vec<GenericArgument*> _generics, Maybe<u64> _genericID,
	           Maybe<OpaqueSubtypeKind> subtypeKind, ir::Mod* _parent, Maybe<usize> _size, VisibilityInfo _visibility,
	           llvm::LLVMContext& llctx, Maybe<MetaInfo> metaInfo);

	static OpaqueType* get(Identifier name, Vec<GenericArgument*> generics, Maybe<u64> genericID,
	                       Maybe<OpaqueSubtypeKind> subtypeKind, ir::Mod* parent, Maybe<usize> size,
	                       VisibilityInfo visibility, llvm::LLVMContext& llCtx, Maybe<MetaInfo> metaInfo);

	String                get_full_name() const;
	Identifier            get_name() const;
	ir::Mod*              get_module() const;
	VisibilityInfo const& get_visibility() const;
	bool                  is_generic() const;
	Maybe<u64>            get_generic_id() const;
	bool                  has_generic_parameter(String const& name) const;
	GenericArgument*      get_generic_parameter(String const& name) const;

	bool  is_subtype_struct() const;
	bool  is_subtype_mix() const;
	bool  is_subtype_unknown() const;
	bool  has_subtype() const;
	void  set_sub_type(ir::ExpandedType* _subTy);
	Type* get_subtype() const;
	bool  has_deduced_size() const;
	usize get_deduced_size() const;

	bool          is_expanded() const final;
	bool          can_be_prerun() const final;
	bool          can_be_prerun_generic() const final;
	Maybe<String> to_prerun_generic_string(ir::PrerunValue* preVal) const final;
	bool          is_type_sized() const final;
	Maybe<bool>   equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
	bool          has_simple_copy() const final;
	bool          has_simple_move() const final;

	bool is_copy_constructible() const final;
	bool is_copy_assignable() const final;
	bool is_move_constructible() const final;
	bool is_move_assignable() const final;
	bool is_destructible() const final;

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	TypeKind type_kind() const final;
	String   to_string() const final;
};

} // namespace qat::ir

#endif
