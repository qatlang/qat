#include "./define_mix_type.hpp"
#include "./expression.hpp"
#include <cmath>

namespace qat::ast {

void DefineMixType::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	mod->entity_name_check(irCtx, name, ir::EntityType::mixType);
	entityState = mod->add_entity(name, ir::EntityType::mixType, this, ir::EmitPhase::phase_2);
}

void DefineMixType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	for (auto& sub : subtypes) {
		if (sub.second.has_value()) {
			sub.second.value()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
			                                        emitCtx);
		}
	}
}

void DefineMixType::create_opaque(ir::Mod* mod, ir::Ctx* irCtx) {
	usize maxSubtypeSize          = 8u;
	bool  foundSizeForAssociation = true;
	usize tagBitwidth             = 1u;
	while (std::pow(2, tagBitwidth) < (subtypes.size() + 1)) {
		tagBitwidth++;
	}
	for (auto& subty : subtypes) {
		if (subty.second.has_value()) {
			auto subTySize = subty.second.value()->get_type_bitsize(EmitCtx::get(irCtx, mod));
			if (subTySize.has_value()) {
				if (maxSubtypeSize < subTySize.value()) {
					maxSubtypeSize = subTySize.value();
				}
			} else {
				foundSizeForAssociation = false;
				break;
			}
		}
	}
	opaquedType = ir::OpaqueType::get(name, {}, None, ir::OpaqueSubtypeKind::MIX, mod,
	                                  foundSizeForAssociation
	                                      ? Maybe<usize>(irCtx->dataLayout.getTypeAllocSizeInBits(llvm::StructType::get(
	                                            irCtx->llctx,
	                                            {llvm::Type::getIntNTy(irCtx->llctx, tagBitwidth),
	                                             llvm::Type::getIntNTy(irCtx->llctx, maxSubtypeSize)},
	                                            isPacked)))
	                                      : None,
	                                  EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), irCtx->llctx, None);
}

void DefineMixType::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (checkResult.has_value() && not checkResult.value()) {
		return;
	} else if (defineChecker) {
		auto checkRes = defineChecker->emit(ctx);
		if (checkRes->get_ir_type()->is_bool()) {
			checkResult = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
			if (not checkResult.value()) {
				return;
			}
		} else {
			ctx->Error("The define condition is expected to be of " + ctx->color("bool") + " type",
			           defineChecker->fileRange);
		}
	}
	if (phase == ir::EmitPhase::phase_1) {
		create_opaque(mod, irCtx);
	} else if (phase == ir::EmitPhase::phase_2) {
		create_type(mod, irCtx);
	}
}

void DefineMixType::create_type(ir::Mod* mod, ir::Ctx* irCtx) {
	Vec<Pair<Identifier, Maybe<ir::Type*>>> subTypesIR;
	bool                                    hasAssociatedType         = false;
	bool                                    allSubtypesHaveSimpleMove = true;
	if (noneVariant.has_value()) {
		subTypesIR.push_back(std::make_pair(Identifier("none", noneVariant.value()), None));
	}
	for (usize i = 0; i < subtypes.size(); i++) {
		for (usize j = i + 1; j < subtypes.size(); j++) {
			if (subtypes.at(i).first.value == subtypes.at(j).first.value) {
				irCtx->Error("The name of this variant of the mix is repeating here. Please "
				             "check logic & make necessary changes",
				             fRanges.at(j));
			}
		}
		if (not hasAssociatedType && subtypes.at(i).second.has_value()) {
			hasAssociatedType = true;
		}
		if (defaultVal.has_value() && (defaultVal.value() == i) && (i != 0 || not noneVariant.has_value()) &&
		    subtypes.at(i).second.has_value()) {
			irCtx->Error("A variant with an associated type cannot be "
			             "used as the default variant of a mix type",
			             FileRange::merge(fRanges.at(i), subtypes.at(i).second.value()->fileRange));
		}
		auto emitCtx = EmitCtx::get(irCtx, mod);
		if (opaquedType) {
			emitCtx->with_opaque_parent(opaquedType);
		}
		auto subTypeTy = subtypes.at(i).second.has_value() ? subtypes.at(i).second.value()->emit(emitCtx) : nullptr;
		if (subtypes.at(i).second.has_value() && subTypeTy->is_opaque() &&
		    not subTypeTy->as_opaque()->is_type_sized()) {
			if (opaquedType && subTypeTy->as_opaque()->is_same(opaquedType)) {
				irCtx->Error(
				    "Type nesting found. The variant " + irCtx->color(subtypes.at(i).first.value) + " of mix type " +
				        irCtx->color(opaquedType->to_string()) +
				        " has the same type associated with it as its parent. Check the code for mistakes or use a pointer"
				        " or reference to the parent type as the associated type",
				    subtypes.at(i).second.value()->fileRange);
			} else {
				irCtx->Error(
				    "The variant " + irCtx->color(subtypes.at(i).first.value) + " of mix type " +
				        irCtx->color(opaquedType->to_string()) +
				        " has an incomplete type with an unknown size associated with it. Such types cannot be used as the"
				        " associated type of a variant",
				    subtypes.at(i).second.value()->fileRange);
			}
		}
		if (subTypeTy && not subTypeTy->has_simple_move()) {
			if (noneVariant.has_value()) {
				irCtx->Error(
				    "The variant " + irCtx->color(subtypes[i].first.value) + " has an associated type of " +
				        irCtx->color(subtypes[i].second.value()->to_string()) +
				        ", which does not have simple-move, but the " + irCtx->color("none") +
				        " variant was requested to be created for this mix type. The " + irCtx->color("none") +
				        " variant can only be created for a mix type if all of the associated types of its variants have simple-move",
				    subtypes[i].second.value()->fileRange);
			}
			allSubtypesHaveSimpleMove = false;
		}
		subTypesIR.push_back(Pair<Identifier, Maybe<ir::Type*>>(
		    subtypes.at(i).first, subtypes.at(i).second.has_value() ? Maybe<ir::Type*>(subTypeTy) : None));
	}
	if (allSubtypesHaveSimpleMove && not noneVariant.has_value()) {
		irCtx->Warning(
		    "Associated types of all variants of this mix type have simple-move. "
		    "This means that this mix type can also have simple-move. But the " +
		        irCtx->color("none") +
		        " variant has not been requested to be created for this mix type. You can do that by adding " +
		        irCtx->color("none") + " as the first variant",
		    fileRange);
	}
	if (not hasAssociatedType) {
		irCtx->Error("No types associated to any of the subfields of the mix type. "
		             "Please change this type to a choice type",
		             fileRange);
	}
	(void)ir::MixType::create(name, opaquedType, {}, mod, subTypesIR, defaultVal, irCtx, noneVariant.has_value(),
	                          isPacked, EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), fileRange, None);
}

Json DefineMixType::to_json() const {
	Vec<JsonValue> subTypesJson;
	for (const auto& sub : subtypes) {
		subTypesJson.push_back(Json()
		                           ._("name", sub.first)
		                           ._("hasType", sub.second.has_value())
		                           ._("type", sub.second.has_value() ? sub.second.value()->to_json() : JsonValue()));
	}
	return Json()
	    ._("nodeType", "defineMixType")
	    ._("name", name)
	    ._("subTypes", subTypesJson)
	    ._("fileRange", fileRange)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue());
}

} // namespace qat::ast
