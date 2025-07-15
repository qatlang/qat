#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "./pointer_owner.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class PtrType final : public Type {
	Type*    type;
	PtrOwner owner;
	bool     isMulti;
	bool     isSubtypeVar;
	bool     isNonNullable;

  public:
	PtrType(Type* _type, bool _isSubtypeVar, PtrOwner _owner, bool _isNonNullable, bool _isMulti,
	        FileRangePtr _fileRange)
	    : Type(_fileRange), type(_type), owner(_owner), isMulti(_isMulti), isSubtypeVar(_isSubtypeVar),
	      isNonNullable(_isNonNullable) {}

	useit static PtrType* create(Type* _type, bool _isSubtypeVar, PtrOwner _owner, bool _isNonNullable, bool _isMulti,
	                             FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(PtrType), _type, _isSubtypeVar, _owner, _isNonNullable, _isMulti,
		                         _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type*   emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final;
	useit Json        to_json() const final;
	useit String      to_string() const final;
};

} // namespace qat::ast

#endif
