#include "./polymorph.hpp"
#include "../../IR/skill.hpp"
#include "../../IR/types/polymorph.hpp"

namespace qat::ast {

Maybe<usize> PolymorphType::get_type_bitsize(EmitCtx* ctx) const {
	auto ptrTy = llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace());
	return (usize)ctx->irCtx->dataLayout.getTypeAllocSizeInBits(isTyped ? llvm::StructType::create({
	                                                                          ptrTy,
	                                                                          ptrTy,
	                                                                      })
	                                                                    : llvm::StructType::create({ptrTy, ptrTy}));
}

ir::Type* PolymorphType::emit(EmitCtx* ctx) {
	Vec<ir::Skill*> irSkills;
	for (auto& sk : skills) {
		irSkills.push_back(sk.find_skill(ctx));
	}
	auto ptrOwner =
	    owner.has_value() ? Maybe<ir::PtrOwner>(get_ptr_owner(ctx, owner.value(), owner.value().range)) : None;
	Maybe<ir::AddressSpace> addr;
	if (addressSpace.has_value()) {
		addr = addressSpace.value().to_ir(ctx);
	}
	return ir::Polymorph::create(isTyped, isVar, std::move(irSkills), std::move(ptrOwner), std::move(addr), ctx->irCtx);
}

} // namespace qat::ast
