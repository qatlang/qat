#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./address_space.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class Function;
class Region;

enum class LocalityKind {
	NONE,
	ANY_REGION,
	REGION_TYPE,
	HEAP,
	OWN,
	SELF,
	STATIC,
	PRERUN,
	USE,
	ATOMIC,
};

class Locality {
  public:
	void*        owner;
	LocalityKind locality;

	static Locality of_heap();
	static Locality of_static();
	static Locality of_none();
	static Locality of_own(Function* fun);
	static Locality of_self(Type* type);
	static Locality of_use();
	static Locality of_atomic();
	static Locality of_region_type(Region* region);
	static Locality of_any_region();
	static Locality of_prerun();

	Type* owner_as_type() const { return (Type*)owner; }

	Region* owner_as_region() const { return ((Type*)owner)->as_region(); }

	Function* owner_as_parent_function() const { return (Function*)owner; }

	Type* owner_as_parent_type() const { return (Type*)owner; }

	bool is_none() const { return locality == LocalityKind::NONE; }

	bool is_any_region() const { return locality == LocalityKind::ANY_REGION; }

	bool is_region_type() const { return locality == LocalityKind::REGION_TYPE; }

	bool is_heap() const { return locality == LocalityKind::HEAP; }

	bool is_own() const { return locality == LocalityKind::OWN; }

	bool is_self() const { return locality == LocalityKind::SELF; }

	bool is_static() const { return locality == LocalityKind::STATIC; }

	bool is_prerun() const { return locality == LocalityKind::PRERUN; }

	bool is_use() const { return locality == LocalityKind::USE; }

	bool is_atomic() const { return locality == LocalityKind::ATOMIC; }

	bool is_same(const Locality& other) const;

	String to_string() const;
};

class PtrType : public Type {
  private:
	Type*               subType;
	bool                isSubtypeVar;
	Locality            locality;
	bool                hasMulti;
	bool                nonNullable;
	Maybe<AddressSpace> addressSpace;

	static Vec<PtrType*> allPtrTypes;

  public:
	PtrType(bool _isSubVar, Type* _subtype, bool nonNullable, Locality _owner, bool _hasMulti,
	        Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx);

	static PtrType* get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, Locality _owner, bool _hasMulti,
	                    Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);

	Type*    get_subtype() const;
	Locality get_locality() const;

	bool has_address_space() const { return addressSpace.has_value(); }

	u32 usable_address_space(ir::Ctx* irCtx) const;

	Maybe<AddressSpace> const& get_address_space() const;

	bool is_subtype_variable() const;
	bool is_multi() const;
	bool is_nullable() const;
	bool is_non_nullable() const;

	bool can_be_prerun() const final { return subType->is_function(); }

	bool is_type_sized() const final;
	bool has_prerun_default_value() const final;
	bool has_simple_copy() const final;
	bool has_simple_move() const final;

	PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	TypeKind type_kind() const override;
	String   to_string() const override;
};

} // namespace qat::ir

#endif
