#include "./plain_initialiser.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/vector.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>

namespace qat::ast {

ir::Value* PlainInitialiser::emit(EmitCtx* ctx) {
	if (not type && not is_type_inferred()) {
		ctx->Error("No type provided for plain initialisation and no type could be inferred from scope", fileRange);
	}
	auto  reqInfo  = ctx->get_access_info();
	auto* typeEmit = type ? type.emit(ctx) : inferredType;
	if (type && is_type_inferred() && not typeEmit->is_same(inferredType)) {
		ctx->Error("The type provided for initialisation is " + ctx->color(typeEmit->to_string()) +
		               ", which does not match the type inferred from scope, which is " +
		               ctx->color(inferredType->to_string()),
		           type.get_range());
	}
	if (typeEmit->is_struct()) {
		auto* cTy = typeEmit->as_struct();
		if (fieldValues.empty()) {
			if (cTy->has_default_constructor()) {
				auto* dFn = cTy->get_default_constructor();
				if (dFn->is_accessible(ctx->get_access_info())) {
					llvm::Value* alloca = nullptr;
					if (isLocalDecl()) {
						type_check_local(cTy, ctx->irCtx, fileRange);
						alloca = localValue->get_llvm();
					} else if (irName) {
						auto newAlloca =
						    ctx->get_fn()->get_block()->new_local(irName->value, cTy, isVar, irName->range);
						alloca = newAlloca->get_llvm();
					} else {
						alloca = ir::Logic::newAlloca(ctx->get_fn(), None, cTy->get_llvm_type());
					}
					(void)dFn->call(ctx->irCtx, {alloca}, None, ctx->mod);
					if (isLocalDecl()) {
						return localValue->to_new_ir_value();
					} else {
						return ir::Value::get(alloca, cTy, true);
					}
				} else {
					ctx->Error("The default constructor of struct type " + ctx->color(cTy->get_full_name()) +
					               " is not accessible here",
					           fileRange);
				}
			}
		}
		if (ctx->get_fn()->is_method() ? not((ir::Method*)ctx->get_fn())->get_parent_type()->is_same(cTy) : true) {
			if (cTy->has_any_constructor()) {
				ctx->Error("Struct type " + ctx->color(cTy->get_full_name()) +
				               " have constructors and hence the plain initialiser "
				               "cannot be used for this type",
				           fileRange);
			} else if (cTy->has_any_from_convertor()) {
				ctx->Error("Struct type " + ctx->color(cTy->get_full_name()) +
				               " has convertor constructors and hence the plain initialiser "
				               "syntax cannot be used for this type",
				           fileRange);
			}
		}
		if (cTy->get_field_count() != fieldValues.size()) {
			ctx->Error("Struct type " + ctx->color(cTy->to_string()) + " has " +
			               ctx->color(std::to_string(cTy->get_field_count())) + " members. But there are only " +
			               ctx->color(std::to_string(fieldValues.size())) + " expressions provided",
			           fileRange);
		}
		if (fields.empty()) {
			for (usize i = 0; i < fieldValues.size(); i++) {
				auto* mem = cTy->get_field_at(i);
				if (not mem->visibility.is_accessible(reqInfo)) {
					ctx->Error("Member " + ctx->color(mem->name.value) + " of struct type " +
					               ctx->color(cTy->get_full_name()) +
					               " is not accessible here and hence plain "
					               "initialiser cannot be used for this type",
					           fieldValues.at(i)->fileRange);
				}
				indices.push_back(i);
			}
		} else {
			for (usize i = 0; i < fields.size(); i++) {
				auto fName  = fields.at(i).first;
				auto fRange = fields.at(i).second;
				if (cTy->has_field_with_name(fName)) {
					auto* mem = cTy->get_field_at(cTy->get_index_of(fName).value());
					if (not mem->visibility.is_accessible(reqInfo)) {
						ctx->Error("The member " + ctx->color(fName) + " of struct type " +
						               ctx->color(cTy->get_full_name()) + " is not accessible here",
						           fRange);
					}
					for (usize j = 0; j != i; j++) {
						if (fields.at(i).first == fields.at(j).first) {
							ctx->Error("The value for member field " + ctx->color(fields.at(i).first) +
							               " of struct type " + ctx->color(cTy->to_string()) +
							               " is already provided. Please check logic",
							           fields.at(i).second);
						}
					}
					indices.push_back(cTy->get_index_of(fields.at(i).first).value());
				} else {
					ctx->Error("The struct type " + ctx->color(cTy->to_string()) + " does not have a member named " +
					               ctx->color(fields.at(i).first),
					           fields.at(i).second);
				}
			}
		}
		// FIXME - Support default values
		Vec<ir::Value*> irVals;
		bool            areAllValsPrerun = true;
		for (usize i = 0; i < fieldValues.size(); i++) {
			if (fieldValues.at(i)->has_type_inferrance()) {
				fieldValues.at(i)->as_type_inferrable()->set_inference_type(cTy->get_field_at(indices.at(i))->type);
			}
			auto* fVal  = fieldValues.at(i)->emit(ctx);
			auto* memTy = cTy->get_field_at(i)->type;
			if (fVal->get_ir_type()->is_same(memTy) ||
			    (memTy->is_ref() && memTy->as_ref()->get_subtype()->is_same(fVal->get_ir_type()) &&
			     fVal->is_ghost_ref() && (memTy->as_ref()->has_variability() ? fVal->is_variable() : true)) ||
			    (fVal->get_ir_type()->is_ref() && fVal->get_ir_type()->as_ref()->get_subtype()->is_same(memTy))) {
				if (not fVal->is_prerun_value()) {
					areAllValsPrerun = false;
				}
				irVals.push_back(fVal);
			} else {
				ctx->Error("This expression does not match the type of the "
				           "corresponding member " +
				               ctx->color(cTy->get_field_name_at(i)) + " of struct type " +
				               ctx->color(cTy->to_string()) + ". The member is of type " +
				               ctx->color(memTy->to_string()) + ", but the expression is of type " +
				               ctx->color(fVal->get_ir_type()->to_string()),
				           fieldValues.at(i)->fileRange);
			}
		}
		llvm::Value* alloca = nullptr;
		if (isLocalDecl()) {
			type_check_local(cTy, ctx->irCtx, fileRange);
			alloca = localValue->get_llvm();
		} else {
			auto newAlloca = ctx->get_fn()->get_block()->new_local(
			    irName.has_value() ? irName->value : ctx->get_fn()->get_random_alloca_name(), cTy, isVar,
			    irName.has_value() ? irName->range : fileRange);
			alloca = newAlloca->get_llvm();
		}
		if (areAllValsPrerun) {
			SHOW("PlainInit Type " << cTy->to_string() << " all values prerun. Count " << indices.size())
			Vec<llvm::Constant*> memVals(indices.size());
			for (usize i = 0; i < indices.size(); i++) {
				memVals[indices[i]] = irVals[i]->get_llvm_constant();
			}
			ctx->irCtx->builder.CreateStore(
			    llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(cTy->get_llvm_type()), memVals), alloca);
		} else {
			for (usize i = 0; i < irVals.size(); i++) {
				auto* memPtr = ctx->irCtx->builder.CreateStructGEP(cTy->get_llvm_type(), alloca, indices.at(i));
				auto  irVal  = irVals.at(i);
				auto  memTy  = cTy->get_field_at(indices.at(i))->type;
				if (irVal->is_ghost_ref() || irVal->is_ref()) {
					if (memTy->has_simple_copy() || memTy->has_simple_move()) {
						if (irVal->is_ref()) {
							irVal->load_ghost_ref(ctx->irCtx->builder);
						}
						auto* irOrigVal = ctx->irCtx->builder.CreateLoad(memTy->get_llvm_type(), irVal->get_llvm());
						if (not memTy->has_simple_copy()) {
							if (irVal->is_ref() && not irVal->get_ir_type()->as_ref()->has_variability()) {
								ctx->Error(
								    "This is a reference without variability and hence simple-move is not possible",
								    fieldValues.at(i)->fileRange);
							} else if (not irVal->is_ref() && not irVal->is_variable()) {
								ctx->Error(
								    "This is an expression without variability and hence simple-move is not possible",
								    fieldValues.at(i)->fileRange);
							}
							ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(memTy->get_llvm_type()),
							                                irVal->get_llvm());
							if (irVal->is_local_value()) {
								ctx->get_fn()->get_block()->add_moved_value(irVal->get_local_id().value());
							}
						}
						ctx->irCtx->builder.CreateStore(irOrigVal, memPtr);
					} else {
						ctx->Error("The member field " + ctx->color(cTy->get_field_name_at(indices.at(i))) +
						               " expects an expression of type " + ctx->color(memTy->to_string()) +
						               " which does not have simple-copy and simple-move. Please use " +
						               ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
						           fieldValues.at(i)->fileRange);
					}
				} else {
					ctx->irCtx->builder.CreateStore(irVal->get_llvm(), memPtr);
				}
			}
		}
		if (isLocalDecl()) {
			return localValue;
		} else {
			return ir::Value::get(alloca, cTy, true);
		}
	} else if (typeEmit->is_text()) {
		if (fieldValues.size() == 2) {
			auto* strData  = fieldValues.at(0)->emit(ctx);
			auto* strLen   = fieldValues.at(1)->emit(ctx);
			auto* dataType = strData->get_ir_type();
			auto* lenType  = strLen->get_ir_type();
			if (dataType->is_ref()) {
				dataType = dataType->as_ref()->get_subtype();
			}
			if (lenType->is_ref()) {
				lenType = lenType->as_ref()->get_subtype();
			}
			if (dataType->is_mark() && dataType->as_mark()->get_subtype()->is_unsigned() &&
			    dataType->as_mark()->get_subtype()->as_unsigned()->is_bitwidth(8u)) {
				if (dataType->as_mark()->is_slice()) {
					// FIXME - Change when `check` is added
					// FIXME - Add length confirmation if pointer is multi, compare with
					// the provided length of the string
					if (strData->is_ref()) {
						strData->load_ghost_ref(ctx->irCtx->builder);
					}
					strData = ir::Value::get(
					    strData->is_value()
					        ? ctx->irCtx->builder.CreateExtractValue(strData->get_llvm(), {0u})
					        : ctx->irCtx->builder.CreateLoad(
					              llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
					                                     ctx->irCtx->dataLayout.getProgramAddressSpace()),
					              ctx->irCtx->builder.CreateStructGEP(dataType->get_llvm_type(), strData->get_llvm(),
					                                                  0u)),
					    ir::MarkType::get(false, ir::UnsignedType::create(8u, ctx->irCtx), false,
					                      ir::MarkOwner::of_anonymous(), false, ctx->irCtx),
					    false);
				} else {
					if (strData->is_ghost_ref() || strData->get_ir_type()->is_ref()) {
						if (strData->is_ref() && strData->is_ghost_ref()) {
							strData->load_ghost_ref(ctx->irCtx->builder);
						}
						strData = ir::Value::get(
						    ctx->irCtx->builder.CreateLoad(dataType->get_llvm_type(), strData->get_llvm()), dataType,
						    false);
					}
				}
				if (lenType->is_unsigned() && lenType->as_unsigned()->get_bitwidth() == 64u) {
					if (strLen->is_ghost_ref() || strLen->is_ref()) {
						strLen =
						    ir::Value::get(ctx->irCtx->builder.CreateLoad(lenType->get_llvm_type(), strLen->get_llvm()),
						                   lenType, false);
					}
					auto* strSliceTy = ir::TextType::get(ctx->irCtx);
					auto* strAlloca  = ir::Logic::newAlloca(ctx->get_fn(), None, strSliceTy->get_llvm_type());
					ctx->irCtx->builder.CreateStore(
					    strData->get_llvm(),
					    ctx->irCtx->builder.CreateStructGEP(strSliceTy->get_llvm_type(), strAlloca, 0));
					ctx->irCtx->builder.CreateStore(strLen->get_llvm(), ctx->irCtx->builder.CreateStructGEP(
					                                                        strSliceTy->get_llvm_type(), strAlloca, 1));
					return ir::Value::get(strAlloca, strSliceTy, false);
				} else {
					ctx->Error("The second argument for creating a string slice is not a 64-bit unsigned integer",
					           fieldValues.at(1)->fileRange);
				}
			} else {
				ctx->Error("You are creating an " + ctx->color("text") +
				               " value from 2 arguments and the first argument "
				               "should be of " +
				               ctx->color("mark:[u8]") + " or " + ctx->color("slice:[u8]") + " type",
				           fieldValues.at(0)->fileRange);
			}
		} else if (fieldValues.size() == 1) {
			auto* strData   = fieldValues.at(0)->emit(ctx);
			auto* strDataTy = strData->get_ir_type();
			if (strDataTy->is_ref()) {
				strDataTy = strDataTy->as_ref()->get_subtype();
			}
			if (strDataTy->is_mark() && strDataTy->as_mark()->is_slice() &&
			    (strDataTy->as_mark()->get_subtype()->is_unsigned() &&
			     strDataTy->as_mark()->get_subtype()->as_unsigned()->is_bitwidth(8u))) {
				auto* strTy = ir::TextType::get(ctx->irCtx);
				if (strData->is_ghost_ref() || strData->is_ref()) {
					if (strData->is_ref()) {
						strData->load_ghost_ref(ctx->irCtx->builder);
					}
					return ir::Value::get(ctx->irCtx->builder.CreatePointerCast(
					                          strData->get_llvm(),
					                          llvm::PointerType::get(strTy->get_llvm_type(),
					                                                 ctx->irCtx->dataLayout.getProgramAddressSpace())),
					                      ir::RefType::get(strData->is_ghost_ref()
					                                           ? strData->is_variable()
					                                           : strData->get_ir_type()->as_ref()->has_variability(),
					                                       strTy, ctx->irCtx),
					                      false);
				} else {
					return ir::Value::get(
					    ctx->irCtx->builder.CreateBitCast(strData->get_llvm(), strDataTy->get_llvm_type()), strDataTy,
					    false);
				}
			} else {
				ctx->Error("While creating a " + ctx->color("text") +
				               " value with one argument, the argument is expected to be of type " +
				               ctx->color("slice:[u8]"),
				           fieldValues.at(0)->fileRange);
			}
		} else {
			ctx->Error(
			    "There are two ways to create a " + ctx->color("text") +
			        " value using a plain initialiser. The first way requires one argument of type " +
			        ctx->color("slice:[u8]") + ". The second way requires 2 arguments. In two possible ways:\n1) " +
			        ctx->color("mark:[u8]") + " and " + ctx->color("u64") + "\n2) " + ctx->color("slice:[u8]") +
			        " and " + ctx->color("u64") +
			        "\nIn case you are providing two arguments, the first argument is supposed to be a mark" +
			        " to the start of the data and the second argument is supposed to be the number of characters," +
			        " EXCLUDING the null character (which is required to be present at the end regardless)",
			    fileRange);
		}
	} else if (typeEmit->is_vector()) {
		auto* vecTy = typeEmit->as_vector();
		if (not fields.empty()) {
			ctx->Error("The type for initialisation is " + ctx->color(vecTy->to_string()) +
			               " so field names cannot be used here",
			           fileRange);
		}
		if (vecTy->is_fixed() && (fieldValues.size() != vecTy->get_count())) {
			ctx->Error("The vectorized type is " + ctx->color(vecTy->to_string()) +
			               " with a fixed length, which expects " + ctx->color(std::to_string(vecTy->get_count())) +
			               " elements, but " + (fieldValues.size() < vecTy->get_count() ? "only " : "") +
			               ctx->color(std::to_string(fieldValues.size())) + " values are provided here",
			           fileRange);
		} else if (vecTy->is_scalable() && ((fieldValues.size() % vecTy->get_count()) != 0)) {
			ctx->Error("The vectorized type is " + ctx->color(vecTy->to_string()) +
			               " which is a scalable vector type, which expects " +
			               ctx->color("n X " + std::to_string(vecTy->get_count())) + " (a multiple of " +
			               ctx->color(std::to_string(vecTy->get_count())) + ") elements. But " +
			               ctx->color(std::to_string(fieldValues.size())) + " values were provided instead",
			           fileRange);
		}
		bool                 areAllValuesConstant = true;
		Vec<ir::Value*>      values;
		Vec<llvm::Constant*> constVals;
		SHOW("Handling field vals")
		for (auto val : fieldValues) {
			if (val->has_type_inferrance()) {
				val->as_type_inferrable()->set_inference_type(vecTy->get_element_type());
			}
			auto irVal = val->emit(ctx);
			auto irTy  = irVal->get_ir_type();
			if (not irVal->is_prerun_value()) {
				areAllValuesConstant = false;
			} else {
				constVals.push_back(irVal->get_llvm_constant());
			}
			if (not irTy->is_same(vecTy->get_element_type()) &&
			    not(irTy->is_ref() && irTy->as_ref()->get_subtype()->is_same(vecTy->get_element_type()))) {
				ctx->Error("Expected an expression of type " + ctx->color(vecTy->get_element_type()->to_string()) +
				               " but got an expression of type " + ctx->color(irTy->to_string()),
				           fileRange);
			}
			SHOW("Done type check")
			if (irTy->is_ref() || irVal->is_ghost_ref()) {
				irTy  = irTy->is_ref() ? irTy->as_ref()->get_subtype() : irTy;
				irVal = ir::Value::get(ctx->irCtx->builder.CreateLoad(irTy->get_llvm_type(), irVal->get_llvm()), irTy,
				                       false);
			}
			SHOW("Pushing value")
			values.push_back(irVal);
		}
		SHOW("Handled all vals")
		if (areAllValuesConstant) {
			if (isLocalDecl()) {
				SHOW("Is local decl and val is const")
				type_check_local(vecTy, ctx->irCtx, fileRange);
				SHOW("Storing vector constant in local")
				ctx->irCtx->builder.CreateStore(llvm::ConstantVector::get(constVals), localValue->get_llvm());
				return localValue->to_new_ir_value();
			} else if (irName.has_value()) {
				auto newAlloca = ctx->get_fn()->get_block()->new_local(irName.value().value, vecTy, isVar, fileRange);
				ctx->irCtx->builder.CreateStore(llvm::ConstantVector::get(constVals), newAlloca->get_llvm());
				return newAlloca->to_new_ir_value();
			} else {
				SHOW("Getting vector " << vecTy->to_string())
				return ir::PrerunValue::get(llvm::ConstantVector::get(constVals), vecTy);
			}
		} else {
			SHOW("Values not constant")
			llvm::Value* lastValue =
			    ctx->irCtx->builder.CreateInsertElement(vecTy->get_llvm_type(), values[0]->get_llvm(), (u64)0);
			for (usize i = 1; i < values.size(); i++) {
				lastValue = ctx->irCtx->builder.CreateInsertElement(lastValue, values[i]->get_llvm(), i);
			}
			if (isLocalDecl()) {
				ctx->irCtx->builder.CreateStore(lastValue, localValue->get_llvm());
				return localValue->to_new_ir_value();
			} else if (irName.has_value()) {
				auto newAlloca = ctx->get_fn()->get_block()->new_local(irName.value().value, vecTy, isVar, fileRange);
				ctx->irCtx->builder.CreateStore(lastValue, localValue->get_llvm());
				return newAlloca->to_new_ir_value();
			} else {
				return ir::Value::get(lastValue, vecTy, false);
			}
		}
	} else {
		ctx->Error("The type is " + ctx->color(typeEmit->to_string()) +
		               " and hence cannot be used in a plain initialiser",
		           fileRange);
	}
	return nullptr;
}

Json PlainInitialiser::to_json() const {
	return Json()
	    ._("nodeType", "plainInitialiser")
	    ._("hasType", (bool)type)
	    ._("type", (JsonValue)type)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
