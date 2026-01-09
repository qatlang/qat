#ifndef QAT_TYPES_REFERENCE_HPP
#define QAT_TYPES_REFERENCE_HPP

#include "./address_space.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class RefType final : public Type {
  private:
	Type*               type;
	bool                isSubtypeVar;
	Maybe<AddressSpace> addressSpace;

  public:
	RefType(Type* _type, bool _isSubtypeVar, Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Type(std::move(_fileRange)), type(_type), isSubtypeVar(_isSubtypeVar),
	      addressSpace(std::move(_addressSpace)) {}

	static RefType* create(Type* type, bool isSubtypeVar, Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(RefType), type, isSubtypeVar, std::move(addressSpace), std::move(fileRange));
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
