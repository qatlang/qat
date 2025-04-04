#include "./define_choice_type.hpp"
#include "./types/qat_type.hpp"
#include "expression.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

void DefineChoiceType::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	mod->entity_name_check(irCtx, name, ir::EntityType::choiceType);
	entityState = mod->add_entity(name, ir::EntityType::choiceType, this, ir::EmitPhase::phase_1);
}

void DefineChoiceType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	if (providedIntegerTy.has_value()) {
		providedIntegerTy.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState,
		                                               EmitCtx::get(irCtx, mod));
	}
	for (auto& field : fields) {
		if (field.second.has_value()) {
			field.second.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState,
			                                          EmitCtx::get(irCtx, mod));
		}
	}
}

void DefineChoiceType::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	Vec<Vec<Identifier>>           fieldNames;
	Maybe<Vec<llvm::ConstantInt*>> fieldValues;
	ir::Type*                      variantValueType = nullptr;
	Maybe<llvm::ConstantInt*>      lastVal;
	Maybe<ir::Type*>               providedType;

	auto emitCtx = EmitCtx::get(irCtx, mod);
	SHOW("Checking provided integer type")
	if (providedIntegerTy) {
		providedType = providedIntegerTy.value()->emit(EmitCtx::get(irCtx, mod));
		if (not providedType.value()->is_integer() && not providedType.value()->is_unsigned() &&
		    not(providedType.value()->is_native_type() &&
		        (providedType.value()->as_native_type()->get_subtype()->is_unsigned() ||
		         providedType.value()->as_native_type()->get_subtype()->is_integer()))) {
			irCtx->Error("Choice types can only have an integer or unsigned integer as its underlying type",
			             providedIntegerTy.value()->fileRange);
		}
	}
	for (usize i = 0; i < fields.size(); i++) {
		for (usize j = i + 1; j < fields.size(); j++) {
			for (usize k = 0; k < fields[j].first.size(); k++) {
				for (usize l = 0; l < fields[i].first.size(); l++) {
					if (fields[j].first[k].value == fields[i].first[l].value) {
						irCtx->Error("Name of the choice variant is repeating here", fields[j].first[k].range,
						             std::make_pair("The first occurence can be found here", fields[i].first[l].range));
					}
				}
			}
		}
		fieldNames.push_back(fields.at(i).first);
		if (fields.at(i).second.has_value()) {
			if (providedType && fields.at(i).second.value()->has_type_inferrance()) {
				fields.at(i).second.value()->as_type_inferrable()->set_inference_type(providedType.value());
			}
			auto iVal   = fields.at(i).second.value()->emit(EmitCtx::get(irCtx, mod));
			auto iValTy = iVal->get_ir_type();
			if (not iValTy->is_integer() && not iValTy->is_unsigned() &&
			    not(iValTy->is_native_type() && (iValTy->as_native_type()->get_subtype()->is_unsigned() ||
			                                     iValTy->as_native_type()->get_subtype()->is_integer()))) {
				irCtx->Error(
				    "The value for a variant of this choice type should be of integer or unsigned integer type. Got an expression of type " +
				        irCtx->color(iValTy->to_string()),
				    fields.at(i).second.value()->fileRange);
			}
			if (iValTy->is_native_type()) {
				iValTy = iValTy->as_native_type()->get_subtype();
			}
			if (providedType.has_value() && not iVal->get_ir_type()->is_same(providedType.value())) {
				irCtx->Error("The provided value of the variant of this choice type has type " +
				                 irCtx->color(iVal->get_ir_type()->to_string()) +
				                 ", which does not match the provided underlying type " +
				                 irCtx->color(providedType.value()->to_string()),
				             fields.at(i).second.value()->fileRange);
			} else if (not iVal->get_ir_type()->is_integer() && not iVal->get_ir_type()->is_unsigned() &&
			           not(iVal->get_ir_type()->is_native_type() &&
			               (iVal->get_ir_type()->as_native_type()->get_subtype()->is_unsigned() ||
			                iVal->get_ir_type()->as_native_type()->get_subtype()->is_integer()))) {
				irCtx->Error("Value for variant for choice type should either be a signed or unsigned integer type",
				             fields.at(i).second.value()->fileRange);
			}
			if (variantValueType && not variantValueType->is_same(iVal->get_ir_type())) {
				irCtx->Error("Value provided for this variant does not match the type of the previous values",
				             fields.at(i).second.value()->fileRange);
			} else if (not variantValueType) {
				variantValueType = iVal->get_ir_type();
			}
			llvm::ConstantInt* fieldResult =
			    llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(iVal->get_llvm_constant(), irCtx->dataLayout));
			if ((not lastVal.has_value()) && (i > 0)) {
				Vec<llvm::ConstantInt*> prevVals; // In reverse
				for (usize j = i - 1; j >= 0; j--) {
					if ((prevVals.empty() ? fieldResult : prevVals.back())->isMinValue(iValTy->is_integer()) &&
					    j != 0) {
						irCtx->Error(
						    "Tried to calculate values for variants before " +
						        irCtx->color(fields.at(i).first.front().value) +
						        ". Could not calculate underlying value for the variant " +
						        irCtx->color(fields.at(j).first.front().value) + " as the variant " +
						        irCtx->color(fields.at(j + 1).first.front().value) +
						        " right after this variant has the minimum value possible for the underlying type " +
						        irCtx->color(providedType.has_value() ? providedType.value()->to_string()
						                                              : iVal->get_ir_type()->to_string()) +
						        " of this choice type.",
						    fields.at(j).first.front().range);
					}
					prevVals.push_back(llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
					    llvm::ConstantExpr::getSub(
					        fieldResult, llvm::ConstantInt::get(fieldResult->getType(), 1u, iValTy->is_integer())),
					    irCtx->dataLayout)));
					if (prevVals.back()->isNegative()) {
						areValuesUnsigned = false;
					}
				}
				fieldValues = Vec<llvm::ConstantInt*>{};
				fieldValues.value().insert(fieldValues.value().end(), prevVals.end(), prevVals.begin());
			}
			lastVal = fieldResult;
			SHOW("Index for field " << fields.at(i).first.front().value << " is "
			                        << *lastVal.value()->getValue().getRawData())
			if (not fieldValues.has_value()) {
				fieldValues = Vec<llvm::ConstantInt*>{};
			}
			SHOW("Pushing field value")
			if (lastVal.value()->isNegative()) {
				areValuesUnsigned = false;
			}
			fieldValues->push_back(lastVal.value());
		} else if (lastVal.has_value()) {
			if (lastVal.value()->isMaxValue(variantValueType->is_integer())) {
				irCtx->Error("Could not calculate value for the variant " +
				                 irCtx->color(fields[i].first.front().value) + " as the variant " +
				                 irCtx->color(fields[i - 1].first.front().value) +
				                 " before this has the maximum value possible for the underlying type " +
				                 irCtx->color(providedType.has_value() ? providedType.value()->to_string()
				                                                       : variantValueType->to_string()) +
				                 " of this choice type",
				             fields[i].first.front().range);
			}
			SHOW("Last value has value")
			auto newVal = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
			    llvm::ConstantExpr::getAdd(lastVal.value(), llvm::ConstantInt::get(lastVal.value()->getType(), 1u,
			                                                                       variantValueType->is_integer())),
			    irCtx->dataLayout));
			SHOW("Index for field " << fields.at(i).first.front().value << " is " << newVal)
			if (newVal->isNegative()) {
				areValuesUnsigned = false;
			}
			fieldValues->push_back(newVal);
			lastVal = newVal;
		}
	}
	if (fieldValues.has_value()) {
		for (usize i = 0; i < fieldValues->size(); i++) {
			for (usize j = i + 1; j < fieldValues->size(); j++) {
				if (fieldValues->at(i) == fieldValues->at(j)) {
					irCtx->Error("Underlying value for the variant " + irCtx->color(fields.at(j).first.front().value) +
					                 " is repeating. Please check logic and make necessary changes",
					             fields.at(j).first.front().range);
				}
			}
		}
	}
	SHOW("Creating choice type in the IR")
	(void)ir::ChoiceType::create(name, mod, std::move(fieldNames), std::move(fieldValues), providedType,
	                             areValuesUnsigned, defaultVal, emitCtx->get_visibility_info(visibSpec), irCtx,
	                             fileRange, None);
	SHOW("Created choice type")
}

Json DefineChoiceType::to_json() const {
	Vec<JsonValue> fieldsJson;
	for (const auto& field : fields) {
		Vec<JsonValue> namesJSON;
		for (auto& name : field.first) {
			namesJSON.push_back(name);
		}
		fieldsJson.push_back(
		    Json()
		        ._("name", namesJSON)
		        ._("hasValue", field.second.has_value())
		        ._("value", field.second.has_value() ? field.second.value()->to_json() : JsonValue())
		        ._("valueRange", field.second.has_value() ? field.second.value()->fileRange : JsonValue()));
	}
	return Json()
	    ._("name", name)
	    ._("fields", fieldsJson)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
