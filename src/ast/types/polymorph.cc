#include "./polymorph.hpp"
#include "../../IR/skill.hpp"
#include "../../IR/types/polymorph.hpp"

namespace qat::ast {

Maybe<usize> PolymorphType::get_type_bitsize(EmitCtx* ctx) const {
	auto ptrTy = llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace());
	return (usize)ctx->irCtx->dataLayout.getTypeAllocSizeInBits(
	    isTyped ? llvm::StructType::create({ptrTy, ptrTy, ptrTy}) : llvm::StructType::create({ptrTy, ptrTy}));
}

ir::Type* PolymorphType::emit(EmitCtx* ctx) {
	Vec<ir::Skill*> irSkills;
	for (auto& sk : skills) {
		irSkills.push_back(sk.find_skill(ctx));
	}
	auto ptrOwner =
	    ownRange.has_value() ? get_ptr_owner(ctx, ownType, None, ownRange.value()) : ir::PtrOwner::of_anonymous();
	return ir::Polymorph::create(isTyped, std::move(irSkills), ptrOwner, ctx->irCtx);
}

} // namespace qat::ast
