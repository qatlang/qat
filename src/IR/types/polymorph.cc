#include "./polymorph.hpp"
#include "../../utils/utils.hpp"
#include "../context.hpp"
#include "../skill.hpp"
#include "./unsigned.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

Polymorph::Polymorph(bool _isTyped, bool _isVar, Vec<Skill*> _skills, PtrOwner _owner, ir::Ctx* ctx)
    : isTyped(_isTyped), isVar(_isVar), skills(std::move(skills)), owner(_owner) {
	auto             objPtrTy = ir::PtrType::get(isVar, ir::UnsignedType::create(8u, ctx), true, owner, false, ctx);
	auto             ptrTy    = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace());
	Vec<llvm::Type*> subTys   = {objPtrTy->get_llvm_type()};
	subTys.reserve(skills.size() + 2);
	linkingName += "qat'poly_";
	if (isTyped) {
		subTys.push_back(ptrTy);
		linkingName += "typed";
	}
	linkingName += ":[";
	if (isVar) {
		linkingName += "var ";
	}
	for (usize i = 0; i < skills.size(); i++) {
		subTys.push_back(ptrTy);
		linkingName += skills[i]->get_link_names().toName();
		if (i != (skills.size() - 1)) {
			linkingName += ", ";
		}
	}
	linkingName += "]";
	llvmType = llvm::StructType::create(subTys, linkingName, false);
}

Polymorph* Polymorph::create(bool isTyped, bool isVar, Vec<Skill*> skills, PtrOwner owner, ir::Ctx* ctx) {
	std::sort(skills.begin(), skills.end(), [](Skill* first, Skill* second) -> bool {
		return utils::bytewise_comparison(first->get_full_name(), second->get_full_name());
	});
	for (auto* type : allTypes) {
		if (type->is_poly() && (type->as_poly()->get_skills().size() == skills.size())) {
			auto exPoly = type->as_poly();
			if (exPoly->isTyped != isTyped) {
				continue;
			}
			if (exPoly->isVar != isVar) {
				continue;
			}
			if (not exPoly->owner.is_same(owner)) {
				continue;
			}
			bool sameSkills = true;
			for (usize i = 0; i < exPoly->get_skills().size(); i++) {
				if (exPoly->get_skills()[i]->get_id() != skills[i]->get_id()) {
					sameSkills = false;
					break;
				}
			}
			if (sameSkills) {
				return exPoly;
			}
		}
	}
	return std::construct_at(OwnNormal(Polymorph), isTyped, isVar, std::move(skills), owner, ctx);
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
	return String(isTyped ? "poly:[type, " : "poly:[") + (isVar ? "var " : "") + skillStr + "]";
}

} // namespace qat::ir
