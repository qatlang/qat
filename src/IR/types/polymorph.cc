#include "./polymorph.hpp"
#include "../../utils/utils.hpp"
#include "../context.hpp"
#include "../skill.hpp"
#include "./unsigned.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

Polymorph::Polymorph(bool _isTyped, bool _isVar, Vec<Skill*> _skills, Maybe<PtrOwner> _owner, ir::Ctx* ctx)
    : isTyped(_isTyped), isVar(_isVar), skills(std::move(skills)), owner(std::move(_owner)) {
	auto objPtrTy    = ir::PtrType::get(isVar, ir::UnsignedType::create(8u, ctx), true,
	                                    owner.value_or(PtrOwner::of_anonymous()), false, ctx);
	auto ptrTy       = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace());
	auto globalPtrTy = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getDefaultGlobalsAddressSpace());

	Vec<llvm::Type*> subTys = {objPtrTy->get_llvm_type()};
	subTys.reserve(skills.size() + 2);
	linkingName += "qat'poly_";
	if (isTyped) {
		subTys.push_back(globalPtrTy);
		linkingName += "typed";
	}
	if (owner.has_value()) {
		linkingName += ":ptr";
	}
	linkingName += ":[";
	if (isVar) {
		linkingName += "var ";
	}
	for (usize i = 0; i < skills.size(); i++) {
		subTys.push_back(globalPtrTy);
		linkingName += skills[i]->get_link_names().toName();
		if (i != (skills.size() - 1)) {
			linkingName += ", ";
		}
	}
	linkingName += "]";
	llvmType = llvm::StructType::create(subTys, linkingName, false);
}

Polymorph* Polymorph::create(bool isTyped, bool isVar, Vec<Skill*> skills, Maybe<PtrOwner> owner, ir::Ctx* ctx) {
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
			if (exPoly->owner.has_value() && owner.has_value() && not exPoly->owner.value().is_same(owner.value())) {
				continue;
			} else if (exPoly->owner.has_value() != owner.has_value()) {
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
	return std::construct_at(OwnNormal(Polymorph), isTyped, isVar, std::move(skills), std::move(owner), ctx);
}

String Polymorph::to_string() const {
	String skillStr;
	for (usize i = 0; i < skills.size(); i++) {
		skillStr += skills[i]->get_full_name();
		if (i != (skills.size() - 1)) {
			skillStr += " + ";
		}
	}
	if (owner.has_value() && not owner.value().is_of_anonymous()) {
		skillStr += ", ";
		skillStr += owner.value().to_string();
	}
	return String(isTyped ? (owner.has_value() ? "poly:ptr:[type, " : "poly:[type, ")
	                      : (owner.has_value() ? "poly:ptr:[" : "poly:[")) +
	       (isVar ? "var " : "") + skillStr + "]";
}

} // namespace qat::ir
