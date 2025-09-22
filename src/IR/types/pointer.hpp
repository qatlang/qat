#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./address_space.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class Function;
class Region;

enum class OwnerKind {
	NONE,
	ANY_REGION,
	REGION_TYPE,
	HEAP,
	OWN,
	SELF,
	STATIC,
};

class PtrOwner {
  public:
	void*     owner;
	OwnerKind ownerTy;

	useit static PtrOwner of_heap();
	useit static PtrOwner of_static();
	useit static PtrOwner of_none();
	useit static PtrOwner of_own(Function* fun);
	useit static PtrOwner of_self(Type* type);
	useit static PtrOwner of_region_type(Region* region);
	useit static PtrOwner of_any_region();

	useit Type* owner_as_type() const { return (Type*)owner; }

	useit Region* owner_as_region() const { return ((Type*)owner)->as_region(); }

	useit Function* owner_as_parent_function() const { return (Function*)owner; }

	useit Type* owner_as_parent_type() const { return (Type*)owner; }

	useit bool is_none() const { return ownerTy == OwnerKind::NONE; }

	useit bool is_any_region() const { return ownerTy == OwnerKind::ANY_REGION; }

	useit bool is_region_type() const { return ownerTy == OwnerKind::REGION_TYPE; }

	useit bool is_heap() const { return ownerTy == OwnerKind::HEAP; }

	useit bool is_own() const { return ownerTy == OwnerKind::OWN; }

	useit bool is_self() const { return ownerTy == OwnerKind::SELF; }

	useit bool is_static() const { return ownerTy == OwnerKind::STATIC; }

	useit bool is_same(const PtrOwner& other) const;

	useit String to_string() const;
};

class PtrType : public Type {
  private:
	Type*               subType;
	bool                isSubtypeVar;
	PtrOwner            owner;
	bool                hasMulti;
	bool                nonNullable;
	Maybe<AddressSpace> addressSpace;

	static Vec<PtrType*> allPtrTypes;

  public:
	PtrType(bool _isSubVar, Type* _subtype, bool nonNullable, PtrOwner _owner, bool _hasMulti,
	        Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx);

	useit static PtrType* get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, PtrOwner _owner, bool _hasMulti,
	                          Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);

	useit Type*    get_subtype() const;
	useit PtrOwner get_owner() const;

	useit bool has_address_space() const { return addressSpace.has_value(); }

	useit Maybe<AddressSpace> const& get_address_space() const;

	useit bool is_subtype_variable() const;
	useit bool is_multi() const;
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
