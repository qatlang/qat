#ifndef QAT_AST_MAYBE_HPP
#define QAT_AST_MAYBE_HPP

#include "qat_type.hpp"
namespace qat::ast {

class MaybeType final : public Type {
  private:
	Type* subTyp;
	bool  isPacked;

  public:
	MaybeType(bool _isPacked, Type* _subType, FileRange _fileRange)
		: Type(_fileRange), subTyp(_subType), isPacked(_isPacked) {}

	useit static MaybeType* create(bool _isPacked, Type* _subType, FileRange _fileRange) {
		return std::construct_at(OwnNormal(MaybeType), _isPacked, _subType, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
							 EmitCtx* ctx) final;

	useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;
	useit ir::Type*	  emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final { return AstTypeKind::MAYBE; }
	useit Json		  to_json() const final;
	useit String	  to_string() const final;
};

} // namespace qat::ast

#endif
