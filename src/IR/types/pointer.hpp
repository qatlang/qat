#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./qat_type.hpp"

namespace qat::ir {

class Function;
class Region;

enum class PointerOwnerType {
	anyRegion,
	region,
	heap,
	anonymous,
	type,
	parentFunction,
	parentInstance,
	Static,
};

class MarkOwner {
  public:
	void*            owner;
	PointerOwnerType ownerTy;

	useit static MarkOwner of_heap();
	useit static MarkOwner of_anonymous();
	useit static MarkOwner of_type(Type* type);
	useit static MarkOwner of_parent_function(Function* fun);
	useit static MarkOwner of_parent_instance(Type* type);
	useit static MarkOwner of_region(Region* region);
	useit static MarkOwner of_any_region();

	useit Type* owner_as_type() const { return (Type*)owner; }

	useit Region* owner_as_region() const { return ((Type*)owner)->as_region(); }

	useit Function* owner_as_parent_function() const { return (Function*)owner; }

	useit Type* owner_as_parent_type() const { return (Type*)owner; }

	useit bool is_of_type() const { return ownerTy == PointerOwnerType::type; }

	useit bool is_of_anonymous() const { return ownerTy == PointerOwnerType::anonymous; }

	useit bool is_of_any_region() const { return ownerTy == PointerOwnerType::anyRegion; }

	useit bool is_of_region() const { return ownerTy == PointerOwnerType::region; }

	useit bool is_of_heap() const { return ownerTy == PointerOwnerType::heap; }

	useit bool is_of_parent_function() const { return ownerTy == PointerOwnerType::parentFunction; }

	useit bool is_of_parent_instance() const { return ownerTy == PointerOwnerType::parentInstance; }

	useit bool is_of_static() const { return ownerTy == PointerOwnerType::Static; }

	useit bool is_same(const MarkOwner& other) const;

	useit String to_string() const;
};

class MarkType : public Type {
  private:
	Type*     subType;
	bool      isSubtypeVar;
	MarkOwner owner;
	bool      hasMulti;
	bool      nonNullable;

  public:
	MarkType(bool _isSubVar, Type* _subtype, bool nonNullable, MarkOwner _owner, bool _hasMulti, ir::Ctx* irCtx);

	useit static MarkType* get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, MarkOwner _owner,
	                           bool _hasMulti, ir::Ctx* irCtx);

	useit Type*     get_subtype() const;
	useit MarkOwner get_owner() const;

	useit bool is_subtype_variable() const;
	useit bool is_slice() const;
	useit bool is_nullable() const;
	useit bool is_non_nullable() const;

	useit bool can_be_prerun() const final { return subType->is_function(); }

	useit bool is_type_sized() const final;
	useit bool has_prerun_default_value() const final;
	useit bool has_simple_copy() const final;
	useit bool has_simple_move() const final;

	useit PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	useit TypeKind type_kind() const override;
	useit String   to_string() const override;
};

} // namespace qat::ir

#endif
