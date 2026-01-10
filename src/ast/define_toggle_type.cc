#include "./define_toggle_type.hpp"
#include "../IR/types/toggle.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

void DefineToggleType::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, parent);
	if (defineChecker != nullptr) {
		defineChecker->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	if (genericConstraint != nullptr) {
		genericConstraint->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	for (auto* gen : generics) {
		gen->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	for (auto& var : variants) {
		var.second->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
	}
}

void DefineToggleType::create_opaque(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* mod, ir::Ctx* irCtx) {
	Vec<ir::GenericArgument*> genericsIR;
	for (auto gen : generics) {
		if (not gen->isSet()) {
			if (gen->is_typed()) {
				irCtx->Error("No type is set for the generic type parameter " + irCtx->color(gen->get_name().value) +
				                 " and there is no default type provided",
				             gen->get_range());
			} else if (gen->is_prerun()) {
				irCtx->Error("No value is set for the generic prerun parameter " + irCtx->color(gen->get_name().value) +
				                 " and there is no default expression provided",
				             gen->get_range());
			} else {
				irCtx->Error("Invalid generic kind", gen->get_range());
			}
		}
		genericsIR.push_back(gen->toIRGenericType());
	}
	set_opaque(ir::OpaqueType::get(name, genericsIR, None, ir::OpaqueSubtypeKind::TOGGLE, mod, None,
	                               EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), irCtx->llctx, None));
	if (genericToggleType) {
		genericToggleType->opaqueVariants.push_back(ir::GenericVariant<ir::OpaqueType>(get_opaque(), genericsToFill));
	}
}

ir::ToggleType* DefineToggleType::create_type(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* mod,
                                              ir::Ctx* irCtx) {
	Vec<ir::GenericArgument*> genericsIR;
	for (auto gen : generics) {
		if (not gen->isSet()) {
			if (gen->is_typed()) {
				irCtx->Error("No type is set for the generic type parameter " + irCtx->color(gen->get_name().value) +
				                 " and there is no default type provided",
				             gen->get_range());
			} else if (gen->is_prerun()) {
				irCtx->Error("No value is set for the generic prerun parameter " + irCtx->color(gen->get_name().value) +
				                 " and there is no default expression provided",
				             gen->get_range());
			} else {
				irCtx->Error("Invalid generic kind", gen->get_range());
			}
		}
		genericsIR.push_back(gen->toIRGenericType());
	}
	auto globalCtx = EmitCtx::get(irCtx, mod);
	if (is_generic()) {
		create_opaque(genericsToFill, mod, irCtx);
	}
	auto typeCtx = EmitCtx::get(irCtx, mod)->with_opaque_parent(get_opaque());

	Vec<Pair<Vec<Identifier>, ir::Type*>> variantsIR;
	for (usize i = 0; i < variants.size(); i++) {
		auto& currVariant = variants[i];
		for (usize j = 0; j < currVariant.first.size(); j++) {
			auto& itName = variants[i].first[j];
			for (usize k = j + 1; k < variants[i].first.size(); k++) {
				if (itName.value == variants[i].first[k].value) {
					irCtx->Error("The variant name " + irCtx->color(itName.value) + " is repeating here",
					             variants[i].first[k].range,
					             std::make_pair("The previous occurrence is present here", itName.range));
				}
			}
			for (usize k = i + 1; k < variants.size(); k++) {
				auto& itVariant = variants[k];
				for (usize p = 0; p < itVariant.first.size(); p++) {
					if (itName.value == itVariant.first[p].value) {
						irCtx->Error("The variant name " + irCtx->color(itName.value) + " is repeating here",
						             itVariant.first[p].range,
						             std::make_pair("The previous occurence is present here", itName.range));
					}
				}
			}
		}
		auto varType = currVariant.second->emit(typeCtx);
		if (varType->is_opaque() && (varType->as_opaque()->get_id() == get_opaque()->get_id())) {
			irCtx->Error("The parent toggle type is used here, and this is not allowed", variants[i].second->fileRange);
		}
		if (not varType->has_simple_copy()) {
			irCtx->Error("The associated type " + irCtx->color(varType->to_string()) + " of the variant " +
			                 irCtx->color(currVariant.first.front().value) +
			                 " does not have simple-copy and hence cannot be used for a variant in a toggle type",
			             currVariant.second->fileRange);
		}
		if (not varType->has_simple_move()) {
			irCtx->Error("The associated type " + irCtx->color(varType->to_string()) + " of the variant " +
			                 irCtx->color(currVariant.first.front().value) +
			                 " does not have simple-move and hence cannot be used for a variant in a toggle type",
			             currVariant.second->fileRange);
		}
		variantsIR.push_back(std::make_pair(currVariant.first, varType));
	}
	auto resType = ir::ToggleType::create(name, genericsIR, variantsIR, get_opaque(), mod,
	                                      globalCtx->get_visibility_info(visibSpec), std::move(metaIR), irCtx);
	if (genericToggleType) {
		genericToggleType->variants.push_back(ir::GenericVariant<ir::ToggleType>(resType, genericsToFill));
	}
	if (genericToggleType) {
		for (auto item = genericToggleType->opaqueVariants.begin(); item != genericToggleType->opaqueVariants.end();
		     item++) {
			if (item->get()->get_id() == get_opaque()->get_id()) {
				genericToggleType->opaqueVariants.erase(item);
				break;
			}
		}
	}
	unset_opaque();
	return resType;
}

void DefineToggleType::create_entity(ir::Mod* parent, ir::Ctx* irCtx) {
	if (is_generic()) {
		parent->entity_name_check(irCtx, name, ir::EntityType::genericToggleType);
		entityState = parent->add_entity(name, ir::EntityType::genericToggleType, this, ir::EmitPhase::phase_1);
	} else {
		parent->entity_name_check(irCtx, name, ir::EntityType::toggleType);
		entityState = parent->add_entity(name, ir::EntityType::toggleType, this, ir::EmitPhase::phase_2);
		entityState->phaseToPartial = ir::EmitPhase::phase_1;
	}
}

void DefineToggleType::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
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
	if (metaInfo.has_value() && (not metaIR.has_value())) {
		metaIR = metaInfo.value().toIR(EmitCtx::get(irCtx, mod));
		if (metaIR->has_key(ir::MetaInfo::packedKey)) {
			auto packVal = metaIR->get_value_for(ir::MetaInfo::packedKey);
			if (not packVal->get_ir_type()->is_bool()) {
				irCtx->Error("The key " + irCtx->color(String(ir::MetaInfo::packedKey)) + " expects a value of type " +
				                 irCtx->color("bool"),
				             metaInfo.value().fileRange);
			}
		}
	}

	if (is_generic()) {
		for (auto* gen : generics) {
			gen->emit(ctx);
		}
		genericToggleType = ir::GenericToggleType::create(name, generics, genericConstraint, this, mod,
		                                                  ctx->get_visibility_info(visibSpec));
	} else {
		if (phase == ir::EmitPhase::phase_1) {
			create_opaque({}, mod, irCtx);
		} else if (phase == ir::EmitPhase::phase_2) {
			(void)create_type({}, mod, irCtx);
		}
	}
}

} // namespace qat::ast
