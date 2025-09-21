#include "./slice.hpp"
#include "../context.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

Vec<SliceType*> SliceType::allSliceTypes = {};

SliceType::SliceType(bool _isVar, Type* _subType, ir::Ctx* ctx) : subType(_subType), isVar(_isVar) {
	linkingName = "qat'slice:[" + String(isVar ? "var " : "") + subType->get_name_for_linking() + "]";
	llvmType    = llvm::StructType::create(
        {llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace()),
	        llvm::Type::getIntNTy(ctx->llctx, ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSizeType()))},
        linkingName);
	allSliceTypes.push_back(this);
}

SliceType* SliceType::get(bool isVar, Type* subType, ir::Ctx* ctx) {
	for (auto* type : allSliceTypes) {
		if (type->get_subtype()->is_same(subType) && (type->has_var() == isVar)) {
			return type;
		}
	}
	return std::construct_at(OwnNormal(SliceType), isVar, subType, ctx);
}

String SliceType::to_string() const { return "slice:[" + String(isVar ? "var " : "") + subType->to_string() + "]"; }

} // namespace qat::ir
