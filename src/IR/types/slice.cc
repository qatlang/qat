#include "./slice.hpp"
#include "../context.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

SliceType::SliceType(bool _isVar, Type* _subType, ir::Ctx* ctx) : isVar(_isVar), subType(_subType) {
	linkingName = "qat'slice:[" + String(isVar ? "var " : "") + subType->get_name_for_linking() + "]";
	llvmType    = llvm::StructType::create(
        {llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace()),
	        llvm::Type::getIntNTy(ctx->llctx, ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSizeType()))},
        linkingName);
}

SliceType* SliceType::get(bool isVar, Type* subType, ir::Ctx* ctx) {
	for (auto* type : allTypes) {
		if (type->is_slice() && type->as_slice()->get_subtype()->is_same(subType) &&
		    (type->as_slice()->has_var() == isVar)) {
			return type->as_slice();
		}
	}
	return std::construct_at(OwnNormal(SliceType), isVar, subType, ctx);
}

String SliceType::to_string() const { return "slice:[" + String(isVar ? "var " : "") + subType->to_string() + "]"; }

} // namespace qat::ir
