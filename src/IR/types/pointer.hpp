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
	void*        origin;
	LocalityKind locality;

	static Locality in_heap();
	static Locality in_static();
	static Locality none();
	static Locality in_own();
	static Locality in_self(Type* type);
	static Locality in_use();
	static Locality in_atomic();
	static Locality in_region_type(Region* region);
	static Locality in_any_region();
	static Locality in_prerun();

	Type* origin_as_type() const { return (Type*)origin; }

	Region* origin_as_region() const { return ((Type*)origin)->as_region(); }

	Type* origin_as_parent_type() const { return (Type*)origin; }

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

	bool requires_destruction() const {
		return locality == LocalityKind::OWN or locality == LocalityKind::USE or locality == LocalityKind::ATOMIC;
	}

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
	PtrType(bool _isSubVar, Type* _subtype, bool nonNullable, Locality _locality, bool _hasMulti,
	        Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx);

	static PtrType* get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, Locality _locality, bool _hasMulti,
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
