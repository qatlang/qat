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
	STATIC,
	PRERUN,
	USE,
	ATOMIC,
};

class Locality {
  public:
	void*        origin;
	LocalityKind kind;

	static Locality in_heap();
	static Locality in_static();
	static Locality none();
	static Locality in_own();
	static Locality in_use();
	static Locality in_atomic();
	static Locality in_region_type(Region* region);
	static Locality in_any_region();
	static Locality in_prerun();

	Region* origin_as_region() const { return ((Type*)origin)->as_region(); }

	bool is_none() const { return kind == LocalityKind::NONE; }

	bool is_any_region() const { return kind == LocalityKind::ANY_REGION; }

	bool is_region_type() const { return kind == LocalityKind::REGION_TYPE; }

	bool is_heap() const { return kind == LocalityKind::HEAP; }

	bool is_own() const { return kind == LocalityKind::OWN; }

	bool is_static() const { return kind == LocalityKind::STATIC; }

	bool is_prerun() const { return kind == LocalityKind::PRERUN; }

	bool is_use() const { return kind == LocalityKind::USE; }

	bool is_atomic() const { return kind == LocalityKind::ATOMIC; }

	bool requires_destruction() const {
		return kind == LocalityKind::OWN or kind == LocalityKind::USE or kind == LocalityKind::ATOMIC;
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

	Type* get_subtype() const { return subType; }

	Locality get_locality() const { return locality; }

	bool has_address_space() const { return addressSpace.has_value(); }

	u32 usable_address_space(ir::Ctx* irCtx) const;

	Maybe<AddressSpace> const& get_address_space() const { return addressSpace; }

	bool is_subtype_variable() const { return isSubtypeVar; }

	bool is_multi() const { return hasMulti; }

	bool is_nullable() const { return not(nonNullable); }

	bool is_non_nullable() const { return nonNullable; }

	bool can_be_prerun() const final { return locality.kind == LocalityKind::PRERUN; }

	bool is_copy_constructible() const final {
		return locality.kind == LocalityKind::USE or locality.kind == LocalityKind::ATOMIC;
	}

	bool is_copy_assignable() const final {
		return locality.kind == LocalityKind::USE or locality.kind == LocalityKind::ATOMIC;
	}

	bool is_move_constructible() const final {
		switch (locality.kind) {
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC: {
				return is_nullable();
			}
			default: {
				return false;
			}
		}
	}

	bool is_move_assignable() const final {
		switch (locality.kind) {
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC: {
				return is_nullable();
			}
			default: {
				return false;
			}
		}
	}

	bool is_destructible() const final {
		switch (locality.kind) {
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC: {
				return true;
			}
			default: {
				return false;
			}
		}
	}

	bool is_type_sized() const final { return true; }

	bool has_prerun_default_value() const final { return is_nullable(); }

	bool has_simple_copy() const final {
		switch (locality.kind) {
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC: {
				return false;
			}
			default: {
				return true;
			}
		}
	}

	bool has_simple_move() const final {
		switch (locality.kind) {
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC: {
				return false;
			}
			default: {
				return is_nullable();
			}
		}
	}

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	TypeKind type_kind() const override;
	String   to_string() const override;
};

} // namespace qat::ir

#endif
