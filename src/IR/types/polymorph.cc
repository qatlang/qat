#include "./polymorph.hpp"
#include "../../utils/utils.hpp"
#include "../context.hpp"
#include "../skill.hpp"
#include "./unsigned.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

Vec<Polymorph*> Polymorph::allPolyTypes = {};

Polymorph::Polymorph(bool _isTyped, bool _isVar, Vec<Skill*> _skills, Maybe<Locality> _locality,
                     Maybe<ir::AddressSpace> _addressSpace, ir::Ctx* ctx)
    : isTyped(_isTyped), isVar(_isVar), skills(std::move(_skills)), locality(std::move(_locality)),
      addressSpace(std::move(_addressSpace)) {
	auto objPtrTy = ir::PtrType::get(isVar, ir::UnsignedType::create(8u, ctx), true,
	                                 locality.value_or(Locality::of_none()), false, addressSpace, ctx);
	// auto ptrTy       = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getProgramAddressSpace());
	auto globalPtrTy = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getDefaultGlobalsAddressSpace());

	Vec<llvm::Type*> subTys = {objPtrTy->get_llvm_type()};
	subTys.reserve(skills.size() + 2);
	linkingName += "qat'poly_";
	if (isTyped) {
		subTys.push_back(globalPtrTy);
		linkingName += "typed";
	}
	if (locality.has_value()) {
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

Polymorph* Polymorph::create(bool isTyped, bool isVar, Vec<Skill*> skills, Maybe<Locality> locality,
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
			if (type->locality.has_value() && locality.has_value() &&
			    not type->locality.value().is_same(locality.value())) {
				continue;
			} else if (type->locality.has_value() != locality.has_value()) {
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
	return std::construct_at(OwnNormal(Polymorph), isTyped, isVar, std::move(skills), std::move(locality),
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
	if (locality.has_value() && not locality.value().is_none()) {
		skillStr += ", ";
		skillStr += locality.value().to_string();
	}
	if (addressSpace.has_value()) {
		skillStr += ", ";
		skillStr += addressSpace.value().to_string();
	}
	return String(isTyped ? (locality.has_value() ? "poly:ptr:[type, " : "poly:[type, ")
	                      : (locality.has_value() ? "poly:ptr:[" : "poly:[")) +
	       (isVar ? "var " : "") + skillStr + "]";
}

} // namespace qat::ir
