#ifndef QAT_AST_MAYBE_HPP
#define QAT_AST_MAYBE_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class MaybeType final : public Type {
  private:
	Type* subTyp;
	bool  isPacked;

  public:
	MaybeType(bool _isPacked, Type* _subType, FileRangePtr _fileRange)
	    : Type(_fileRange), subTyp(_subType), isPacked(_isPacked) {}

	static MaybeType* create(bool _isPacked, Type* _subType, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(MaybeType), _isPacked, _subType, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final { return AstTypeKind::MAYBE; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
