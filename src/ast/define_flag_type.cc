#include "./define_flag_type.hpp"
#include "../IR/types/flag.hpp"
#include "../IR/types/unsigned.hpp"
#include "./expression.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

void DefineFlagType::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	mod->entity_name_check(irCtx, name, ir::EntityType::flagType);
	entityState = mod->add_entity(name, ir::EntityType::flagType, this, ir::EmitPhase::phase_1);
}

void DefineFlagType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (providedType) {
		providedType->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	for (auto& var : variants) {
		if (var.value) {
			var.value->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
		}
	}
}

void DefineFlagType::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) {
	if (phase == ir::EmitPhase::phase_1) {
		bool                  foundOneValue = false;
		Vec<ir::FlagVariant>  variantsList(variants.size());
		Vec<ir::PrerunValue*> valuesList(variants.size());
		auto*                 ctx       = EmitCtx::get(irCtx, parent);
		ir::UnsignedType*     underType = nullptr;
		if (providedType) {
			auto provTy = providedType->emit(ctx);
			if (not provTy->is_unsigned()) {
				irCtx->Error(
				    "The provided underlying type of this flag type is " + irCtx->color(provTy->to_string()) +
				        ", which is not an unsigned integer type. The underlying type must be an unsigned integer type",
				    providedType->fileRange);
			}
			underType = provTy->as_unsigned();
		} else {
			underType = ir::UnsignedType::create(variants.size(), irCtx);
		}
		for (usize i = 0; i < variants.size(); i++) {
			if (variants[i].value && (not foundOneValue) && (i > 0)) {
				irCtx->Error(
				    "The variant " + irCtx->color(variants[i].names.front().value) +
				        " has a value provided here, but variants found before this variant has no values provided. Either values should be provided for all variants, or no values should be provided at all",
				    variants[i].value->fileRange);
			} else if ((not variants[i].value) && foundOneValue) {
				irCtx->Error(
				    "The variant " + irCtx->color(variants[i].names.front().value) +
				        " does not have a value provided, but previous variants have values provided. Either values should be provided for all variants, or no values should be provided at all",
				    variants[i].range);
			}
			if (variants[i].value) {
				if (not foundOneValue) {
					foundOneValue = true;
				}
				valuesList.push_back(variants[i].value->emit(ctx));
				if (not valuesList.back()->get_ir_type()->is_same(underType)) {
					irCtx->Error("The value provided here is of type " +
					                 irCtx->color(valuesList.back()->get_ir_type()->to_string()) +
					                 ", but expected a value of type " + irCtx->color(underType->to_string()) +
					                 " instead, which is the underlying type of this flag type",
					             variants[i].value->fileRange);
				}
			}
			for (usize j = i + 1; j < variants.size(); j++) {
				for (usize k = 0; k < variants[i].names.size(); k++) {
					for (usize l = k + 1; l < variants[i].names.size(); l++) {
						if (variants[i].names[k].value == variants[i].names[l].value) {
							irCtx->Error(
							    "The name " + irCtx->color(variants[i].names[k].value) +
							        " is repeating here for the same variant",
							    variants[i].names[l].range,
							    std::make_pair("The first occurrence can be found here", variants[i].names[k].range));
						}
					}
					for (auto& it : variants[j].names) {
						if (variants[i].names[k].value == it.value) {
							irCtx->Error(
							    "The variant name " + irCtx->color(it.value) + " is repeating here", it.range,
							    std::make_pair("The first occurrence can be found here", variants[i].names[k].range));
						}
					}
				}
			}
			variantsList.push_back(ir::FlagVariant{.names = variants[i].names, .isDefault = false});
		}
		(void)ir::FlagType::create(name, parent, std::move(variantsList),
		                           foundOneValue ? Maybe<Vec<ir::PrerunValue*>>(valuesList) : None, underType,
		                           fileRange, ctx->get_visibility_info(visibSpec));
	}
}

} // namespace qat::ast
