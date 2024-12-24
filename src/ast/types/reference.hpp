#ifndef QAT_TYPES_REFERENCE_HPP
#define QAT_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include <string>

namespace qat::ast {

class ReferenceType final : public Type {
  private:
	Type* type;
	bool  isSubtypeVar;

  public:
	ReferenceType(Type* _type, bool _isSubtypeVar, FileRange _fileRange)
	    : Type(_fileRange), type(_type), isSubtypeVar(_isSubtypeVar) {}

	useit static ReferenceType* create(Type* _type, bool _isSubtypeVar, FileRange _fileRange) {
		return std::construct_at(OwnNormal(ReferenceType), _type, _isSubtypeVar, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final;

	useit Json to_json() const final;

	useit String to_string() const final;
};

} // namespace qat::ast

#endif
