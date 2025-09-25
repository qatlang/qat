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

	useit static RefType* get(bool isSubtypeVariable, Type* subtype, Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx);

	useit Type* get_subtype() const;

	useit Maybe<AddressSpace> const& get_address_space() const { return addressSpace; }

	useit bool has_variability() const;

	useit bool is_type_sized() const final;

	useit TypeKind type_kind() const final;

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
