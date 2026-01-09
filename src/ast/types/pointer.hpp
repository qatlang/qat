#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "./address_space.hpp"
#include "./pointer_owner.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class PtrType final : public Type {
	Type*               type;
	PtrOwner            owner;
	bool                isMulti;
	bool                isSubtypeVar;
	bool                isNonNullable;
	Maybe<AddressSpace> addressSpace;

  public:
	PtrType(Type* _type, bool _isSubtypeVar, PtrOwner _owner, bool _isNonNullable, bool _isMulti,
	        Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Type(_fileRange), type(_type), owner(_owner), isMulti(_isMulti), isSubtypeVar(_isSubtypeVar),
	      isNonNullable(_isNonNullable), addressSpace(std::move(_addressSpace)) {}

	useit static PtrType* create(Type* type, bool isSubtypeVar, PtrOwner owner, bool isNonNullable, bool isMulti,
	                             Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PtrType), type, isSubtypeVar, owner, isNonNullable, isMulti,
		                         std::move(addressSpace), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final;

	useit String to_string() const final;
};

} // namespace qat::ast

#endif
