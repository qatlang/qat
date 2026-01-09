#ifndef QAT_IR_TYPES_REFERENCE_HPP
#define QAT_IR_TYPES_REFERENCE_HPP

#include "./address_space.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class RefType : public Type {
	Type*               subType;
	bool                isSubVariable;
	Maybe<AddressSpace> addressSpace;

	static Vec<RefType*> allRefTypes;

  public:
	RefType(bool isSubtypeVariable, Type* _type, Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx);

	static RefType* get(bool isSubtypeVariable, Type* subtype, Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);

	Type* get_subtype() const;

	Maybe<AddressSpace> const& get_address_space() const { return addressSpace; }

	bool has_variability() const;

	bool is_type_sized() const final;

	TypeKind type_kind() const final;

	String to_string() const final;
};

} // namespace qat::ir

#endif
