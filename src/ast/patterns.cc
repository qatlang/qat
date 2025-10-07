#include "./patterns.hpp"
#include "../IR/types/array.hpp"
#include "../IR/types/choice.hpp"
#include "../IR/types/flag.hpp"
#include "../IR/types/maybe.hpp"
#include "../IR/types/mix.hpp"
#include "../IR/types/qat_type.hpp"
#include "../IR/types/reference.hpp"
#include "../IR/types/result.hpp"
#include "../IR/types/tuple.hpp"
#include "../IR/types/unsigned.hpp"
#include "../IR/types/void.hpp"
#include "../IR/value.hpp"
#include "./emit_ctx.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

PatternFill* PatternFill::create_for_type(EmitCtx* ctx, ir::Type* type) {
	auto* res = std::construct_at(OwnNormal(PatternFill), type);
	if (type->is_array()) {
		auto elemFill = create_for_type(ctx, type->as_array()->get_element_type());
		for (usize i = 0; i < type->as_array()->get_length(); i++) {
			res->childFills.push_back(elemFill);
		}
	} else if (type->is_tuple()) {
		for (u32 i = 0; i < type->as_tuple()->get_element_count(); i++) {
			res->childFills.push_back(create_for_type(ctx, type->as_tuple()->get_type_at(i)));
		}
	} else if (type->is_maybe()) {
		res->childFills.push_back(create_for_type(ctx, type->as_maybe()->get_subtype()));
	} else if (type->is_mix()) {
		for (usize i = 0; i < type->as_mix()->get_variant_count(); i++) {
			if (type->as_mix()->get_variant_type_at(i)) {
				res->childFills.push_back(create_for_type(ctx, type->as_mix()->get_variant_type_at(i)));
			} else {
				res->childFills.push_back(create_for_type(ctx, ir::VoidType::get(ctx->irCtx->llctx)));
			}
		}
	} else {
		if (type->is_void()) {
			res->fillType = PatternFillType::COMPLETE;
		}
	}
	return res;
}

void PatternChild::check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const {
	if (is_binding()) {
		auto& bind = as_binding();
		if (isPartOfChain) {
			ctx->Error("Bindings are not allowed in a pattern chain, please remove this or remove the chain",
			           bind.range);
		}
		if (not arm.isRef) {
			if (bind.bindType != BindingType::VALUED) {
				ctx->Error(
				    "The parent expression being matched is not reference-like, hence this binding cannot be used "
				    "here to be bound to a reference-like expression. If you intended to bind within this pattern as a value, then change this to " +
				        ctx->color("use " + bind.name.value) + " instead",
				    bind.range);
			} else if (not fill->type->has_simple_copy()) {
				ctx->Error("The type of the expression here, to be bound as a value is " +
				               ctx->color(fill->type->to_string()) + ", which does not have simple-copy. " +
				               ctx->color(bind.to_string()) +
				               " cannot be used here, as the type is required to have simple-copy. The parent "
				               "expression to be matched can be allocated before being matched, which would cause "
				               "this to bind to a reference, which is done by default by the way. Try using " +
				               ctx->color("'let") +
				               " after the parent expression to allocate the value in-place before matching",
				           bind.range);
			}
		} else if (bind.bindType == BindingType::VARIATION && not ctx->has_pre_call_state()) {
			if (fill->type->is_ref() && not fill->type->as_ref()->has_variability()) {
				ctx->Error("The type of the value being matched at this point is " +
				               ctx->color(fill->type->to_string()) +
				               " which is a reference without variability, but the binding " +
				               ctx->color(bind.to_string()) + " requires the reference to have variability.",
				           bind.range);
			} else if (not arm.isRefVar) {
				ctx->Error("The binding is " + ctx->color(bind.to_string()) +
				               " which requires the parent expression to be a reference with variability.",
				           bind.range);
			}
		}
		if (ctx->has_pre_call_state() && (bind.bindType == BindingType::VALUED)) {
			ctx->Error("The parent is a prerun function and hence the " + ctx->color(bind.to_string()) +
			               " cannot be used here",
			           bind.range);
		}
		fill->fillType = PatternFillType::COMPLETE;
	} else {
		as_pattern()->check(fill, isPartOfChain, arm, ctx);
	}
}

void PatternChild::match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const {
	if (is_pattern()) {
		if (ctx->has_fn()) {
			arm.get_condition_block()->set_active(ctx->irCtx->builder);
		}
		as_pattern()->match(fill, value, arm, ctx);
	} else {
		auto& bind = as_binding();
		if (ctx->has_pre_call_state()) {
			arm.as_prerun_block()->add_local(ir::PrerunLocal::get(
			    bind.name, value->get_ir_type(), bind.bindType == BindingType::VARIATION, value->get_llvm_constant()));
		} else {
			arm.as_block()->set_active(ctx->irCtx->builder);
			if (bind.bindType == BindingType::VALUED) {
				auto resVal = value;
				while (resVal->get_ir_type()->is_ref()) {
					resVal = ir::Value::get(
					    ctx->irCtx->builder.CreateLoad(resVal->get_ir_type()->as_ref()->get_subtype()->get_llvm_type(),
					                                   resVal->get_llvm()),
					    resVal->get_ir_type()->as_ref()->get_subtype(), false);
				}
				(void)arm.as_block()->create_use_value(bind.name.value, resVal->get_llvm(), fill->type, bind.range);
			} else {
				(void)arm.as_block()->create_use_value(
				    bind.name.value, value->get_llvm(),
				    ir::RefType::get(bind.bindType == BindingType::VARIATION, fill->type,
				                     value->extract_address_space(ctx->irCtx), ctx->irCtx),
				    bind.range);
			}
		}
	}
}

void Pattern::precheck(PatternFill* fill, EmitCtx* ctx) const {
	if (fill->fillType == PatternFillType::COMPLETE) {
		ctx->Error("All possible patterns for the type " + ctx->color(fill->type->to_string()) +
		               " have been matched at this point",
		           range);
	}
}

void PatternArray::check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const {
	if (not fill->type->is_array()) {
		ctx->Error("An array pattern is used here, but the type of the expression to be matched for this pattern is " +
		               ctx->color(fill->type->to_string()) + ", which is not an array type",
		           range);
	}
	precheck(fill, ctx);
	auto arrTy = fill->type->as_array();
	if (ellipsis.has_value()) {
		if (patterns.size() >= arrTy->get_length()) {
			ctx->Error("Found ... in the array pattern, but found " + std::to_string(patterns.size()) +
			               " patterns in total. When ... is used, a maximum of only " +
			               std::to_string(arrTy->get_length() - 1) +
			               " patterns can be used for an expression of this type",
			           ellipsis.value().second);
		}
		if (ellipsis.value().first == patterns.size()) {
			for (usize i = patterns.size(); i < arrTy->get_length(); i++) {
				fill->childFills[i]->fillType = PatternFillType::COMPLETE;
			}
			for (usize i = 0; i < patterns.size(); i++) {
				patternIndices.push_back(i);
			}
		} else if (ellipsis.value().first == 0) {
			for (usize i = 0; i < (arrTy->get_length() - patterns.size()); i++) {
				fill->childFills[i]->fillType = PatternFillType::COMPLETE;
			}
			for (usize i = (arrTy->get_length() - patterns.size()); i < arrTy->get_length(); i++) {
				patternIndices.push_back(i);
			}
		} else {
			// Took way too much time to figure out the following - may be I am getting old
			// patterns.size() - ellipsis.value().first is the number of patterns after the ellipsis
			// We subtract the above from the total array length to figure out the extent upto which the patterns should
			// be marked as completed
			for (usize i = ellipsis.value().first; i < (arrTy->get_length() - patterns.size() + ellipsis.value().first);
			     i++) {
				fill->childFills[i]->fillType = PatternFillType::COMPLETE;
			}
			for (usize i = 0; i < ellipsis.value().first; i++) {
				patternIndices.push_back(i);
			}
			for (usize i = (arrTy->get_length() - patterns.size() + ellipsis.value().first); i < arrTy->get_length();
			     i++) {
				patternIndices.push_back(i);
			}
		}
	} else {
		if (patterns.size() < arrTy->get_length()) {
			ctx->Error("The array type of the expression to be matched for this pattern is " +
			               ctx->color(arrTy->to_string()) + ", which expects " + std::to_string(arrTy->get_length()) +
			               " patterns. If you intend to ignore the remaining elements, please use ... at the end",
			           range);
		} else if (patterns.size() > arrTy->get_length()) {
			ctx->Error("The array type of the expression to be matched for this pattern is " +
			               ctx->color(arrTy->to_string()) + ", which expects only " +
			               std::to_string(arrTy->get_length()) + " patterns. But found " +
			               std::to_string(patterns.size()) +
			               " patterns instead. Please remove the additional patterns, or check the logic",
			           range);
		}
		for (usize i = 0; i < patterns.size(); i++) {
			patternIndices.push_back(i);
		}
	}
	for (usize i = 0; i < fill->childFills.size(); i++) {
		patterns[i].check(fill->childFills[i], isPartOfChain, arm, ctx);
	}
	bool isComplete = true;
	for (auto sub : fill->childFills) {
		if (sub->fillType != PatternFillType::COMPLETE) {
			isComplete = false;
			break;
		}
	}
	if (isComplete) {
		fill->fillType = PatternFillType::COMPLETE;
	}
}

void PatternArray::match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const {
	if (ctx->has_fn() && not value->is_ref() && not value->is_ghost_ref() && not value->is_prerun_value()) {
		value = ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), value->get_ir_type(),
		                                              true, range);
	}
	if (value->is_ref()) {
		value->load_ghost_ref(ctx->irCtx->builder);
	}
	auto arrTy = fill->type->as_array();
	if (value->is_prerun_value()) {
		for (usize i = 0; i < patterns.size(); i++) {
			patterns[i].match(
			    fill->childFills[patternIndices[i]],
			    ir::PrerunValue::get(
			        llvm::cast<llvm::ConstantArray>(value->get_llvm())->getAggregateElement(patternIndices[i]),
			        arrTy->get_element_type()),
			    arm, ctx);
		}
	} else if (value->is_ghost_ref() || value->is_ref()) {
		for (usize i = 0; i < patterns.size(); i++) {
			patterns[i].match(
			    fill->childFills[patternIndices[i]],
			    ir::Value::get(ctx->irCtx->builder.CreateInBoundsGEP(arrTy->get_llvm_type(), value->get_llvm(),
			                                                         {0u, (uint)patternIndices[i]}),
			                   ir::RefType::get(value->is_ref() ? value->get_ir_type()->as_ref()->has_variability()
			                                                    : value->has_variability(),
			                                    arrTy->get_element_type(), value->extract_address_space(ctx->irCtx),
			                                    ctx->irCtx),
			                   true),
			    arm, ctx);
		}
	} else {
		for (usize i = 0; i < patterns.size(); i++) {
			patterns[i].match(
			    fill->childFills[patternIndices[i]],
			    ir::Value::get(ctx->irCtx->builder.CreateExtractValue(value->get_llvm(), {(uint)patternIndices[i]}),
			                   arrTy->get_element_type(), true),
			    arm, ctx);
		}
	}
}

String PatternArray::to_string() const {
	String res("[");
	for (usize i = 0; i < patterns.size(); i++) {
		if (ellipsis.has_value() && (ellipsis->first == i)) {
			res += "..., ";
		}
		res += patterns[i].to_string();
		if (i != (patterns.size() - 1)) {
			res += ", ";
		}
	}
	if (ellipsis.has_value() && (ellipsis->first == patterns.size())) {
		res += ", ...";
	}
	return res;
}

void PatternChoice::check(PatternFill* fill, bool, MatchArm&, EmitCtx* ctx) const {
	if (not fill->type->is_choice()) {
		ctx->Error("A choice pattern is used here, but the type of the expression to be matched for this pattern is " +
		               ctx->color(fill->type->to_string()) + ", which is not a choice type." +
		               (fill->type->is_mix()
		                    ? " You have to add (...) at the end of this pattern to do pattern matching over mix types"
		                    : ""),
		           range);
	}
	precheck(fill, ctx);
	auto chTy = fill->type->as_choice();
	if (not chTy->has_field(name.value)) {
		ctx->Error("The choice type " + ctx->color(chTy->to_string()) + " has no variant named " +
		               ctx->color(name.value) + ". Please remove this, or check the logic",
		           range);
	}
	auto const& allNames     = chTy->get_variant_names(name.value);
	bool        foundName    = false;
	auto        foundVariant = name.value;
	for (auto& val : fill->fills) {
		bool breakOuter = false;
		for (auto& nameVal : allNames) {
			if (val == nameVal.value) {
				foundName    = true;
				foundVariant = nameVal.value;
				breakOuter   = true;
				break;
			}
		}
		if (breakOuter) {
			break;
		}
	}
	if (foundName) {
		ctx->Error("The variant " + ctx->color(name.value) + " of the choice type " + ctx->color(chTy->to_string()) +
		               " has already been matched." +
		               ((foundVariant != name.value)
		                    ? " The variant " + ctx->color(name.value) + " has an alternative name " +
		                          ctx->color(foundVariant) + " which was used in a previous pattern."
		                    : "") +
		               " This pattern is redundant, please remove this",
		           range);
	}
	fill->fills.push_back(name.value);
	if (fill->fills.size() == chTy->get_variant_count()) {
		fill->fillType = PatternFillType::COMPLETE;
	}
}

void PatternChoice::match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const {
	if (arm.get_condition_block()) {
		arm.get_condition_block()->set_active(ctx->irCtx->builder);
	}
	value->load_ghost_ref(ctx->irCtx->builder);
	auto chVal = value->get_llvm();
	if (value->is_ref()) {
		chVal = ctx->irCtx->builder.CreateLoad(fill->type->get_llvm_type(), chVal);
	}
	if (llvm::isa<llvm::Constant>(chVal)) {
		arm.get_slot().conditions.push_back(
		    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::ICMP_EQ, llvm::cast<llvm::Constant>(chVal),
		                                         fill->type->as_choice()->get_value_for(name.value)));
	} else {
		arm.get_slot().conditions.push_back(ctx->irCtx->builder.CreateICmp(
		    llvm::CmpInst::ICMP_EQ, chVal, fill->type->as_choice()->get_value_for(name.value)));
	}
}

void PatternMix::check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const {
	if (not fill->type->is_mix()) {
		ctx->Error("A mix pattern is used here, but the type of the expression to be matched to this pattern is " +
		               ctx->color(fill->type->to_string()) + ", which is not a mix type." +
		               (fill->type->is_choice()
		                    ? " You have to remove " +
		                          ctx->color("(" + (child.has_value() ? child.value().to_string() : "") + ")") +
		                          " at the end of this pattern to do pattern matching over choice types."
		                    : ""),
		           range);
	}
	precheck(fill, ctx);
	auto mxTy   = fill->type->as_mix();
	auto varRes = mxTy->has_variant_with_name(name.value);
	if (not varRes.first) {
		ctx->Error("Mix type " + ctx->color(mxTy->to_string()) + " does not have a variant named " +
		               ctx->color(name.value),
		           name.range);
	}
	if (not varRes.second && child.has_value()) {
		ctx->Error("The variant " + ctx->color(name.value) + " of the mix type " + ctx->color(mxTy->to_string()) +
		               " does not have a type associated with it, and hence cannot have " +
		               ctx->color("(" + ctx->color(child->to_string()) + ")") + " at the end",
		           range);
	}
	auto varInd = mxTy->get_variant_index(name.value);
	if (fill->childFills[varInd]->fillType == PatternFillType::COMPLETE) {
		ctx->Error("The variant " + ctx->color(name.value) +
		               " has already been matched completely, and hence this variant is not allowed at this point",
		           name.range);
	}
	if (child.has_value()) {
		child.value().check(fill->childFills[varInd], isPartOfChain, arm, ctx);
	} else {
		fill->childFills[varInd]->fillType = PatternFillType::COMPLETE;
	}
	bool isComplete = true;
	for (auto* subFill : fill->childFills) {
		if (subFill->fillType != PatternFillType::COMPLETE) {
			isComplete = false;
			break;
		}
	}
	if (isComplete) {
		fill->fillType = PatternFillType::COMPLETE;
	}
}

void PatternMix::match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const {
	llvm::Value* tagVal = nullptr;
	if (arm.get_condition_block()) {
		arm.get_condition_block()->set_active(ctx->irCtx->builder);
	}
	if (value->is_ref()) {
		value->load_ghost_ref(ctx->irCtx->builder);
		tagVal = ctx->irCtx->builder.CreateStructGEP(fill->type->get_llvm_type(), value->get_llvm(), 0u);
	} else if (value->is_ghost_ref()) {
		tagVal = ctx->irCtx->builder.CreateStructGEP(fill->type->get_llvm_type(), value->get_llvm(), 0u);
	} else if (value->is_prerun_value()) {
		tagVal = llvm::cast<llvm::ConstantStruct>(value->get_llvm())->getAggregateElement(0u);
	} else {
		tagVal = ctx->irCtx->builder.CreateExtractValue(value->get_llvm(), {0u});
	}
	// auto mxTy = fill->type->as_mix();
	auto index =
	    llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, fill->type->as_mix()->get_tag_bitwidth()),
	                           fill->type->as_mix()->get_index_of(name.value));
	if (llvm::isa<llvm::Constant>(tagVal)) {
		arm.get_slot().conditions.push_back(
		    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::ICMP_EQ, llvm::cast<llvm::Constant>(tagVal), index));
	} else {
		arm.get_slot().conditions.push_back(ctx->irCtx->builder.CreateICmpEQ(tagVal, index));
	}
}

void PatternChain::check(PatternFill* fill, bool, MatchArm& arm, EmitCtx* ctx) const {
	for (auto& it : patterns) {
		if (not pattern_supports_chaining(it.type)) {
			ctx->Error("This pattern cannot be used in a pattern chain", it.range);
		}
		it.check(fill, true, arm, ctx);
	}
}

void PatternChain::match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const {
	Vec<llvm::Value*> mainConditions;
	bool              areMainCondsPre  = true;
	bool              mainPreCondValue = false;
	for (usize i = 0; i < patterns.size(); i++) {
		if (arm.get_condition_block()) {
			arm.get_condition_block()->set_active(ctx->irCtx->builder);
		}
		arm.conditions.push_back(ConditionSlot{.conditions = {}});
		auto slotIndex = arm.conditions.size();
		patterns[i].match(fill, value, arm, ctx);
		Vec<llvm::Value*> itConditions;
		bool              areAllPre    = true;
		bool              preCondValue = true;
		for (usize j = slotIndex; j < arm.conditions.size(); j++) {
			for (auto* it : arm.conditions[j].conditions) {
				itConditions.push_back(it);
				if (not llvm::isa<llvm::Constant>(it)) {
					areAllPre = false;
				} else {
					if (not llvm::cast<llvm::ConstantInt>(
					            llvm::ConstantFoldConstant(llvm::cast<llvm::Constant>(it), ctx->irCtx->dataLayout))
					            ->getValue()
					            .getBoolValue()) {
						preCondValue = false;
					}
				}
			}
		}
		for (usize j = slotIndex; j < arm.conditions.size(); j++) {
			arm.conditions.pop_back();
		}
		if (not itConditions.empty()) {
			if (not areAllPre) {
				arm.get_condition_block()->set_active(ctx->irCtx->builder);
				areMainCondsPre = false;
				mainConditions.push_back(ctx->irCtx->builder.CreateAnd(itConditions));
			} else {
				mainConditions.push_back(
				    llvm::ConstantInt::getBool(llvm::Type::getInt1Ty(ctx->irCtx->llctx), preCondValue));
				if (preCondValue) {
					mainPreCondValue = true;
				}
			}
		}
	}
	if (not mainConditions.empty()) {
		if (not areMainCondsPre) {
			arm.get_condition_block()->set_active(ctx->irCtx->builder);
			arm.conditions.front().conditions.push_back(ctx->irCtx->builder.CreateOr(mainConditions));
		} else {
			arm.conditions.front().conditions.push_back(
			    llvm::ConstantInt::getBool(llvm::Type::getInt1Ty(ctx->irCtx->llctx), mainPreCondValue));
		}
	}
}

void PatternFlag::check(PatternFill* fill, bool, MatchArm&, EmitCtx* ctx) const {
	if (not fill->type->is_flag()) {
		ctx->Error("A flag pattern is used here, but the type of the expression to be matched at this point is " +
		               ctx->color(fill->type->to_string()),
		           range);
	}
	precheck(fill, ctx);
	auto   flTy = fill->type->as_flag();
	String finalNum(flTy->get_underlying_type()->get_bitwidth(), '0');
	if (flagKind == FlagPatternKind::DEFAULT) {
		if (flTy->has_default_variants()) {
			for (usize i = 0; i < flTy->variants.size(); i++) {
				if (flTy->variants[i].isDefault) {
					finalNum[i] = '1';
				}
			}
		}
	} else if (flagKind != FlagPatternKind::NONE) {
		for (auto& id : names) {
			auto ind = flTy->get_index_of(id.value);
			if (not ind.has_value()) {
				ctx->Error("The flag type " + ctx->color(flTy->to_string()) +
				               " of the expression being matched here does not have a variant named " +
				               ctx->color(id.value),
				           id.range);
			}
			if (finalNum[ind.value()] == '1') {
				ctx->Error("The variant " + ctx->color(id.value) + " is repeating here", id.range);
			}
			finalNum[ind.value()] = '1';
		}
	}
	for (auto& fillIt : fill->fills) {
		if (fillIt == finalNum) {
			ctx->Error("The pattern " + ctx->color(to_string()) + " of the flag type " +
			               ctx->color(fill->type->to_string()) +
			               " has already been matched at this point, so there is no need to do it here",
			           range);
		}
	}
}

void PatternFlag::match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const {
	auto   flTy = fill->type->as_flag();
	String flagNum(flTy->get_underlying_type()->get_bitwidth(), '0');
	if ((flagKind == FlagPatternKind::DEFAULT) && flTy->has_default_variants()) {
		for (usize i = 0; i < flTy->variants.size(); i++) {
			if (flTy->variants[i].isDefault) {
				flagNum[i] = '1';
			}
		}
	} else if (flagKind == FlagPatternKind::VARIANTS) {
		for (auto& id : names) {
			flagNum[flTy->get_index_of(id.value).value()] = '1';
		}
	}
	auto flagVal = llvm::ConstantInt::get(flTy->get_llvm_type(),
	                                      llvm::APInt(flTy->get_underlying_type()->get_bitwidth(), flagNum, 2u));
	if (value->is_prerun_value()) {
		arm.get_slot().conditions.push_back(
		    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::ICMP_EQ, value->get_llvm_constant(), flagVal));
	} else {
		arm.get_condition_block()->set_active(ctx->irCtx->builder);
		auto cand = value->get_llvm();
		if (value->is_ghost_ref()) {
			value->load_ghost_ref(ctx->irCtx->builder);
		}
		if (value->is_ref()) {
			cand = ctx->irCtx->builder.CreateLoad(flTy->get_llvm_type(), cand);
		}
		arm.get_slot().conditions.push_back(ctx->irCtx->builder.CreateICmpEQ(cand, flagVal));
	}
}

void PatternRest::check(PatternFill* fill, bool, MatchArm&, EmitCtx* ctx) const {
	if (fill->fillType == PatternFillType::COMPLETE) {
		ctx->Error("All possible patterns for the type " + ctx->color(fill->type->to_string()) +
		               " has been matched already at this point, so there is no need for this pattern",
		           range);
	}
}

} // namespace qat::ast
