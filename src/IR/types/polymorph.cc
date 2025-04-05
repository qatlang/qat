#include "./polymorph.hpp"
#include "../context.hpp"
#include "../skill.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

Polymorph::Polymorph(bool _isTyped, Vec<Skill*> _skills, PtrOwner _owner, ir::Ctx* ctx)
    : isTyped(_isTyped), skills(std::move(skills)), owner(_owner) {
	auto             ptrTy  = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace());
	Vec<llvm::Type*> subTys = {ptrTy, ptrTy};
	linkingName += "qat'poly_";
	if (isTyped) {
		subTys.push_back(ptrTy);
		linkingName += "typed_";
	}
	linkingName += ":[";
	for (usize i = 0; i < skills.size(); i++) {
		linkingName += skills[i]->get_link_names().toName();
		if (i != (skills.size() - 1)) {
			linkingName += ", ";
		}
	}
	linkingName += "]";
	llvmType = llvm::StructType::create(subTys, linkingName, false);
}

String Polymorph::to_string() const {
	String skillStr;
	for (usize i = 0; i < skills.size(); i++) {
		skillStr += skills[i]->get_full_name();
		if (i != (skills.size() - 1)) {
			skillStr += " + ";
		}
	}
	if (not owner.is_of_anonymous()) {
		skillStr += ", ";
		skillStr += owner.to_string();
	}
	return (isTyped ? "poly:[type, " : "poly:[") + skillStr + "]";
}

} // namespace qat::ir
