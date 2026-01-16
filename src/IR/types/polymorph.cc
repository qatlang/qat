#include "./polymorph.hpp"
#include "../../utils/utils.hpp"
#include "../context.hpp"
#include "../skill.hpp"
#include "./unsigned.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

Vec<Polymorph*> Polymorph::allPolyTypes = {};

Polymorph::Polymorph(bool _isTyped, bool _isVar, Vec<Skill*> _skills, Maybe<Locality> _owner,
                     Maybe<ir::AddressSpace> _addressSpace, ir::Ctx* ctx)
    : isTyped(_isTyped), isVar(_isVar), skills(std::move(_skills)), owner(std::move(_owner)),
      addressSpace(std::move(_addressSpace)) {
	auto objPtrTy = ir::PtrType::get(isVar, ir::UnsignedType::create(8u, ctx), true,
	                                 owner.value_or(Locality::of_none()), false, addressSpace, ctx);
	// auto ptrTy       = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace());
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
	allPolyTypes.push_back(this);
}

Polymorph* Polymorph::create(bool isTyped, bool isVar, Vec<Skill*> skills, Maybe<Locality> owner,
                             Maybe<AddressSpace> addressSpace, ir::Ctx* ctx) {
	std::sort(skills.begin(), skills.end(), [](Skill* first, Skill* second) -> bool {
		return utils::bytewise_comparison(first->get_full_name(), second->get_full_name());
	});
	for (auto* type : allPolyTypes) {
		if (type->get_skills().size() == skills.size()) {
			if (type->isTyped != isTyped) {
				continue;
			}
			if (type->isVar != isVar) {
				continue;
			}
			if (type->owner.has_value() && owner.has_value() && not type->owner.value().is_same(owner.value())) {
				continue;
			} else if (type->owner.has_value() != owner.has_value()) {
				continue;
			}
			bool sameSkills = true;
			for (usize i = 0; i < type->get_skills().size(); i++) {
				if (type->get_skills()[i]->get_id() != skills[i]->get_id()) {
					sameSkills = false;
					break;
				}
			}
			if (sameSkills) {
				return type;
			}
		}
	}
	return std::construct_at(OwnNormal(Polymorph), isTyped, isVar, std::move(skills), std::move(owner),
	                         std::move(addressSpace), ctx);
}

String Polymorph::to_string() const {
	String skillStr;
	for (usize i = 0; i < skills.size(); i++) {
		skillStr += skills[i]->get_full_name();
		if (i != (skills.size() - 1)) {
			skillStr += " + ";
		}
	}
	if (owner.has_value() && not owner.value().is_none()) {
		skillStr += ", ";
		skillStr += owner.value().to_string();
	}
	if (addressSpace.has_value()) {
		skillStr += ", ";
		skillStr += addressSpace.value().to_string();
	}
	return String(isTyped ? (owner.has_value() ? "poly:ptr:[type, " : "poly:[type, ")
	                      : (owner.has_value() ? "poly:ptr:[" : "poly:[")) +
	       (isVar ? "var " : "") + skillStr + "]";
}

} // namespace qat::ir
