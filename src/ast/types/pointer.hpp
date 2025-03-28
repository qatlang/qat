#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "./mark_owner.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class MarkType final : public Type {
	Type*             type;
	MarkOwnType       ownTyp;
	Maybe<ast::Type*> ownerTyTy;
	bool              isSlice;
	bool              isSubtypeVar;
	bool              isNonNullable;

  public:
	MarkType(Type* _type, bool _isSubtypeVar, MarkOwnType _ownTy, bool _isNonNullable, Maybe<Type*> _ownerTyTy,
	         bool _isSlice, FileRange _fileRange)
	    : Type(_fileRange), type(_type), ownTyp(_ownTy), ownerTyTy(_ownerTyTy), isSlice(_isSlice),
	      isSubtypeVar(_isSubtypeVar), isNonNullable(_isNonNullable) {}

	useit static MarkType* create(Type* _type, bool _isSubtypeVar, MarkOwnType _ownTy, bool _isNonNullable,
	                              Maybe<Type*> _ownerTyTy, bool _isSlice, FileRange _fileRange) {
		return std::construct_at(OwnNormal(MarkType), _type, _isSubtypeVar, _ownTy, _isNonNullable, _ownerTyTy,
		                         _isSlice, _fileRange);
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
