#include "./toggle.hpp"
#include "../../ast/define_toggle_type.hpp"
#include "../../ast/expression.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../context.hpp"
#include "../logic.hpp"

namespace qat::ir {

ToggleType::ToggleType(Identifier _name, Vec<GenericArgument*> _generics, Vec<Pair<Vec<Identifier>, Type*>> _variants,
                       ir::OpaqueType* _opaqueType, ir::Mod* _parent, VisibilityInfo const& _visibility,
                       Maybe<MetaInfo> _metaInfo, ir::Ctx* irCtx)
    : ExpandedType(std::move(_name), std::move(_generics), _parent, _visibility), variants(std::move(_variants)),
      metaInfo(std::move(_metaInfo)) {
	opaqueEquivalent                 = _opaqueType;
	usize       candidateSizeInBytes = 0;
	usize       candidateAlign       = 1024;
	llvm::Type* candidateType        = nullptr;
	usize       typeIndex            = 0;
	for (auto& it : variants) {
		auto typeSize  = (usize)irCtx->dataLayout.getTypeStoreSize(it.second->get_llvm_type());
		auto typeAlign = irCtx->dataLayout.getPrefTypeAlign(it.second->get_llvm_type()).value();
		if (typeAlign < candidateAlign) {
			candidateSizeInBytes = typeSize;
			candidateAlign       = typeAlign;
			candidateType        = it.second->get_llvm_type();
			underlyingTypeIndex  = typeIndex;
		}
		if (typeSize > maxVariantByteSize) {
			maxVariantByteSize = typeSize;
		}
		typeIndex++;
	}
	linkingName = get_link_names().toName();
	Vec<llvm::Type*> elements;
	elements.push_back(candidateType);
	if (maxVariantByteSize > candidateSizeInBytes) {
		elements.push_back(
		    llvm::ArrayType::get(llvm::Type::getInt8Ty(irCtx->llctx), maxVariantByteSize - candidateSizeInBytes));
	}
	llvmType = llvm::StructType::create(irCtx->llctx, elements, linkingName);
	opaqueEquivalent->set_sub_type(this);
	if (generics.empty()) {
		parent->toggleTypes.push_back(this);
	}
	hasSimpleCopy = true;
	hasSimpleMove = true;
}

ir::PrerunValue* ToggleType::get_prerun_default_value(ir::Ctx*) {
	return ir::PrerunValue::get(llvm::ConstantAggregateZero::get(llvmType), this);
}

LinkNames ToggleType::get_link_names() const {
	Maybe<String> foreignID;
	Maybe<String> linkAlias;
	if (metaInfo) {
		foreignID = metaInfo->get_foreign_id();
		linkAlias = metaInfo->get_value_as_string_for(ir::MetaInfo::linkAsKey);
	}
	if (not foreignID.has_value()) {
		foreignID = parent->get_relevant_foreign_id();
	}
	auto linkNames = parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::toggle), foreignID);
	if (is_generic()) {
		Vec<LinkNames> genericlinkNames;
		for (auto* param : generics) {
			if (param->is_typed()) {
				genericlinkNames.push_back(
				    LinkNames({LinkNameUnit(param->as_typed()->get_type()->get_name_for_linking(),
				                            LinkUnitType::genericTypeValue)},
				              None, nullptr));
			} else if (param->is_prerun()) {
				auto* preRes = param->as_prerun();
				genericlinkNames.push_back(LinkNames(
				    {LinkNameUnit(preRes->get_type()->to_prerun_generic_string(preRes->get_expression()).value(),
				                  LinkUnitType::genericPrerunValue)},
				    None, nullptr));
			}
		}
		linkNames.addUnit(LinkNameUnit("", LinkUnitType::genericList, genericlinkNames), None);
	}
	linkNames.setLinkAlias(linkAlias);
	return linkNames;
}

GenericToggleType::GenericToggleType(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                     ast::PrerunExpression* _constraint, ast::DefineToggleType* _defineToggleType,
                                     Mod* _parent, VisibilityInfo const& _visibInfo)
    : name(std::move(_name)), generics(std::move(_generics)), defineToggleType(_defineToggleType), parent(_parent),
      visibility(_visibInfo), constraint(_constraint) {
	parent->genericToggleTypes.push_back(this);
}

bool GenericToggleType::all_parameters_have_default() const {
	for (auto* gen : generics) {
		if (not gen->hasDefault()) {
			return false;
		}
	}
	return true;
}

Type* GenericToggleType::fill_generics(Vec<GenericToFill*>& toFillTypes, ir::Ctx* irCtx, FileRangePtr range) {
	for (auto& oVar : opaqueVariants) {
		if (oVar.check(irCtx, [&](String const& msg, FileRangePtr rng) { irCtx->Error(msg, rng); }, toFillTypes)) {
			return oVar.get();
		}
	}
	for (auto& var : variants) {
		if (var.check(irCtx, [&](String const& msg, FileRangePtr rng) { irCtx->Error(msg, rng); }, toFillTypes))
			return var.get();
	}
	auto* ctx = ast::EmitCtx::get(irCtx, parent);
	ir::fill_generics(ctx, generics, toFillTypes, range);
	if (constraint != nullptr) {
		auto checkVal = constraint->emit(ctx);
		if (not checkVal->get_ir_type()->is_bool()) {
			irCtx->Error("The constraints for generic parameters should be of " + irCtx->color("bool") +
			                 " type. Got an expression of " + irCtx->color(checkVal->get_ir_type()->to_string()),
			             constraint->fileRange);
		}
		if (not llvm::cast<llvm::ConstantInt>(checkVal->get_llvm_constant())->getValue().getBoolValue()) {
			irCtx->Error("The provided parameters for the generic struct type do not satisfy the constraints", range,
			             Pair<String, FileRangePtr>{"The constraint can be found here", constraint->fileRange});
		}
	}
	Vec<ir::GenericArgument*> genParams;
	for (auto genAb : generics) {
		genParams.push_back(genAb->toIRGenericType());
	}
	auto variantName = ir::Logic::get_generic_variant_name(name.value, toFillTypes);
	if (variantNames.contains(variantName)) {
		irCtx->Error("Repeating variant name: " + variantName, range);
	}
	variantNames.insert(variantName);
	irCtx->add_active_generic(
	    ir::GenericEntityMarker{variantName, ir::GenericEntityType::toggleType, range, 0u, genParams}, true);
	auto* resultTy = defineToggleType->create_type(toFillTypes, parent, irCtx);
	for (auto* temp : generics) {
		temp->unset();
	}
	if (irCtx->get_active_generic().warningCount > 0) {
		auto count = irCtx->get_active_generic().warningCount;
		irCtx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
		                   " generated while creating generic variant" + irCtx->highlightWarning(variantName),
		               range);
	}
	irCtx->remove_active_generic();
	return resultTy;
}

} // namespace qat::ir
