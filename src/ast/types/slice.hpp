#ifndef QAT_AST_TYPES_SLICE_HPP
#define QAT_AST_TYPES_SLICE_HPP

#include "../expression.hpp"
#include "./address_space.hpp"
#include "./qat_type.hpp"

#include <clang/Basic/AddressSpaces.h>

namespace qat::ast {

class SliceType final : public Type {
	bool                isVar;
	Type*               subType;
	Maybe<AddressSpace> addressSpace;

  public:
	SliceType(bool _isVar, Type* _subType, Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Type(_fileRange), isVar(_isVar), subType(_subType), addressSpace(std::move(_addressSpace)) {}

	useit static SliceType* create(bool isVar, Type* subType, Maybe<AddressSpace> addressSpace,
	                               FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(SliceType), isVar, subType, std::move(addressSpace), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
		subType->update_dependencies(phase, expect, ent, ctx);
		if (addressSpace.has_value() && addressSpace.value().value) {
			UPDATE_DEPS(addressSpace.value().value);
		}
	}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const {
		if (addressSpace.has_value() && addressSpace.value().value) {
			return None;
		}
		return ctx->irCtx->clangTargetInfo->getPointerWidth(clang::getLangASFromTargetAS(
		    addressSpace.has_value() ? addressSpace.value().to_ir(ctx).get_number(ctx->irCtx)
		                             : ctx->irCtx->dataLayout.getProgramAddressSpace()));
	}

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const { return AstTypeKind::SLICE; }

	useit Json to_json() const;

	useit String to_string() const;
};

} // namespace qat::ast

#endif
