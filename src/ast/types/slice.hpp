#ifndef QAT_AST_TYPES_SLICE_HPP
#define QAT_AST_TYPES_SLICE_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class SliceType final : public Type {
	bool  isVar;
	Type* subType;

  public:
	SliceType(bool _isVar, Type* _subType, FileRange _fileRange)
	    : Type(std::move(_fileRange)), isVar(_isVar), subType(_subType) {}

	useit static SliceType* create(bool isVar, Type* subType, FileRange fileRange) {
		return std::construct_at(OwnNormal(SliceType), isVar, subType, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
		subType->update_dependencies(phase, expect, ent, ctx);
	}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const {
		return ctx->irCtx->clangTargetInfo->getPointerWidth(ctx->irCtx->get_language_address_space());
	}

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const { return AstTypeKind::SLICE; }

	useit Json to_json() const;

	useit String to_string() const;
};

} // namespace qat::ast

#endif
