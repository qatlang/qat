#ifndef QAT_AST_TYPES_ERROR_HPP
#define QAT_AST_TYPES_ERROR_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class ErrorType final : public Type {
	bool  hasNoneVariant;
	Type* subType;

  public:
	ErrorType(bool _hasNoneVariant, Type* _subType, FileRangePtr _fileRange)
	    : Type(_fileRange), hasNoneVariant(_hasNoneVariant), subType(_subType) {}

	static ErrorType* create(bool hasNoneVariant, Type* subType, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(ErrorType), hasNoneVariant, subType, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* state,
	                         EmitCtx* ctx) final {
		subType->update_dependencies(phase, dep, state, ctx);
	}

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final { return subType->get_type_bitsize(ctx); }

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final { return AstTypeKind::ERROR; }

	String to_string() const final { return (hasNoneVariant ? "error:[" : "error![") + subType->to_string() + "]"; }
};

} // namespace qat::ast

#endif
