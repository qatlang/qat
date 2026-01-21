#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "./address_space.hpp"
#include "./locality.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class PtrType final : public Type {
	Type*               type;
	Locality            locality;
	bool                isMulti;
	bool                isSubtypeVar;
	bool                isNonNullable;
	Maybe<AddressSpace> addressSpace;

  public:
	PtrType(Type* _type, bool _isSubtypeVar, Locality _locality, bool _isNonNullable, bool _isMulti,
	        Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Type(_fileRange), type(_type), locality(_locality), isMulti(_isMulti), isSubtypeVar(_isSubtypeVar),
	      isNonNullable(_isNonNullable), addressSpace(std::move(_addressSpace)) {}

	static PtrType* create(Type* type, bool isSubtypeVar, Locality locality, bool isNonNullable, bool isMulti,
	                       Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PtrType), type, isSubtypeVar, locality, isNonNullable, isMulti,
		                         std::move(addressSpace), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final;

	String to_string() const final;
};

} // namespace qat::ast

#endif
