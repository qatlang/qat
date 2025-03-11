#include "./logic.hpp"
#include "../ast/emit_ctx.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./control_flow.hpp"
#include "./function.hpp"
#include "./generics.hpp"
#include "./qat_module.hpp"
#include "./stdlib.hpp"
#include "./types/array.hpp"
#include "./types/float.hpp"
#include "./types/integer.hpp"
#include "./types/native_type.hpp"
#include "./types/pointer.hpp"
#include "./types/reference.hpp"
#include "./types/text.hpp"
#include "./types/tuple.hpp"
#include "./types/unsigned.hpp"
#include "./value.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

ir::Value* Logic::handle_pass_semantics(ast::EmitCtx* ctx, ir::Type* expectedType, ir::Value* value,
                                        FileRange valueRange) {
	if (expectedType->is_same(value->get_ir_type())) {
		if (value->is_ghost_ref()) {
			auto valueType = value->get_ir_type();
			if (valueType->has_simple_copy() || valueType->has_simple_move()) {
				auto* loadRes = ctx->irCtx->builder.CreateLoad(valueType->get_llvm_type(), value->get_llvm());
				if (not valueType->has_simple_copy()) {
					if (not value->is_variable()) {
						ctx->Error("This expression does not have variability and hence simple-move is not possible",
						           valueRange);
					}
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(valueType->get_llvm_type()),
					                                value->get_llvm());
				}
				return ir::Value::get(loadRes, valueType, false);
			} else {
				ctx->Error("This expression is of type " + ctx->color(valueType->to_string()) +
				               " which does not have simple-copy and simple-move. Please use " + ctx->color("'copy") +
				               " or " + ctx->color("'move") + " accordingly",
				           valueRange);
			}
		} else {
			return value;
		}
	} else if ((expectedType->is_ref() && expectedType->as_ref()->is_same(value->get_ir_type()) &&
	            (value->is_ghost_ref() && (expectedType->as_ref()->has_variability() ? value->is_variable() : true))) ||
	           (expectedType->is_ref() && value->is_ref() &&
	            expectedType->as_ref()->get_subtype()->is_same(value->get_ir_type()->as_ref()->get_subtype()) &&
	            (expectedType->as_ref()->has_variability() ? value->get_ir_type()->as_ref()->has_variability()
	                                                       : true))) {
		if (value->is_ref()) {
			value->load_ghost_ref(ctx->irCtx->builder);
		}
		return value;
	} else if (value->is_ref() && value->get_ir_type()->as_ref()->get_subtype()->is_same(expectedType)) {
		value->load_ghost_ref(ctx->irCtx->builder);
		auto memType = value->get_ir_type()->as_ref()->get_subtype();
		if (memType->has_simple_copy() || memType->has_simple_move()) {
			auto* loadRes = ctx->irCtx->builder.CreateLoad(memType->get_llvm_type(), value->get_llvm());
			if (not memType->has_simple_copy()) {
				if (not value->get_ir_type()->as_ref()->has_variability()) {
					ctx->Error("This expression is of type " + ctx->color(value->get_ir_type()->to_string()) +
					               " which is a reference without variability and hence simple-move is not possible",
					           valueRange);
				}
			}
			return ir::Value::get(loadRes, memType, false);
		} else {
			ctx->Error("This expression is a reference of type " + ctx->color(memType->to_string()) +
			               " which does not have simple-copy and simple-move. Please use " + ctx->color("'copy") +
			               " or " + ctx->color("'move") + " accordingly",
			           valueRange);
		}
	} else {
		ctx->Error("Expected expression of type " + ctx->color(expectedType->to_string()) +
		               ", but the provided expression is of type " + ctx->color(value->get_ir_type()->to_string()),
		           valueRange);
	}
	return nullptr;
}

ir::Value* Logic::int_to_std_string(bool isSigned, ast::EmitCtx* ctx, ir::Value* value, FileRange fileRange) {
	if (ir::StdLib::is_std_lib_found() &&
	    ir::StdLib::stdLib->has_generic_function(isSigned ? "signed_to_string" : "unsigned_to_string",
	                                             AccessInfo::get_privileged())) {
		auto stringTy      = ir::StdLib::get_string_type();
		auto convGenericFn = ir::StdLib::stdLib->get_generic_function(
		    isSigned ? "signed_to_string" : "unsigned_to_string", AccessInfo::get_privileged());
		auto intTy = value->is_ref() ? value->get_ir_type()->as_ref()->get_subtype() : value->get_ir_type();
		auto convFn =
		    convGenericFn->fill_generics({ir::GenericToFill::GetType(intTy, fileRange)}, ctx->irCtx, fileRange);
		if (value->is_ref() || value->is_ghost_ref()) {
			value->load_ghost_ref(ctx->irCtx->builder);
			if (value->is_ref()) {
				value = ir::Value::get(ctx->irCtx->builder.CreateLoad(intTy->get_llvm_type(), value->get_llvm()), intTy,
				                       false);
			}
		}
		auto strRes = convFn->call(ctx->irCtx, {value->get_llvm()}, None, ctx->mod);
		ctx->irCtx->builder.CreateStore(
		    strRes->get_llvm(), ctx->get_fn()
		                            ->get_block()
		                            ->new_local(ctx->get_fn()->get_random_alloca_name(), stringTy, true, fileRange)
		                            ->get_llvm());
		return strRes;
	} else {
		ctx->Error("Cannot convert integer of type " +
		               ctx->color(value->is_ref() ? value->get_ir_type()->as_ref()->get_subtype()->to_string()
		                                          : value->get_ir_type()->to_string()) +
		               " to " + ctx->color("std:String") + " as the standard library function " +
		               ctx->color(isSigned ? "signed_to_string:[T]" : "unsigned_to_string:[T]") + " could not be found",
		           fileRange);
	}
	return nullptr;
}

Pair<String, Vec<llvm::Value*>> Logic::format_values(ast::EmitCtx* ctx, Vec<ir::Value*> values, Vec<FileRange> ranges,
                                                     FileRange fileRange) {
	Vec<llvm::Value*> printVals;
	String            formatString;

	std::function<void(ir::Value*, FileRange)> formatValue = [&](ir::Value* val, FileRange valRange) {
		auto valTy = val->get_ir_type()->is_ref() ? val->get_ir_type()->as_ref()->get_subtype() : val->get_ir_type();
		if (valTy->is_text()) {
			formatString += "%.*s";
			if (val->is_prerun_value()) {
				printVals.push_back(val->get_llvm_constant()->getAggregateElement(1u));
				printVals.push_back(val->get_llvm_constant()->getAggregateElement(0u));
			} else if (val->is_ghost_ref() || val->is_ref()) {
				auto* strTy = val->is_ref() ? val->get_ir_type()->as_ref()->get_subtype() : val->get_ir_type();
				if (val->is_ref()) {
					val->load_ghost_ref(ctx->irCtx->builder);
				}
				printVals.push_back(ctx->irCtx->builder.CreateLoad(
				    llvm::Type::getInt64Ty(ctx->irCtx->llctx),
				    ctx->irCtx->builder.CreateStructGEP(strTy->get_llvm_type(), val->get_llvm(), 1u)));
				printVals.push_back(ctx->irCtx->builder.CreateLoad(
				    llvm::Type::getInt8Ty(ctx->irCtx->llctx)
				        ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
				    ctx->irCtx->builder.CreateStructGEP(strTy->get_llvm_type(), val->get_llvm(), 0u)));
			} else {
				printVals.push_back(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {1u}));
				printVals.push_back(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}));
			}
		} else if (valTy->is_native_type() && valTy->as_native_type()->is_cstring()) {
			formatString += "%s";
			if (val->is_ref() || val->is_ghost_ref()) {
				val->load_ghost_ref(ctx->irCtx->builder);
				if (val->is_ref()) {
					val = ir::Value::get(ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), val->get_llvm()),
					                     val->get_ir_type()->as_ref()->get_subtype(), false);
				}
			}
			printVals.push_back(val->get_llvm());
		} else if (valTy->is_integer() ||
		           (valTy->is_native_type() && valTy->as_native_type()->get_subtype()->is_integer())) {
			if (val->is_prerun_value()) {
				auto valStr = valTy->to_prerun_generic_string(val->as_prerun()).value();
				formatString += valStr;
			} else {
				auto intTy =
				    valTy->is_integer() ? valTy->as_integer() : valTy->as_native_type()->get_subtype()->as_integer();
				// auto strVal =
				//     int_to_std_string(true, ctx, ir::Value::get(val->get_llvm(), intTy, false),
				//     valRange);
				// formatString += "%.*s";
				// printVals.push_back(ctx->irCtx->builder.CreateExtractValue(strVal->get_llvm(), {1u}));
				// printVals.push_back(ctx->irCtx->builder.CreateExtractValue(strVal->get_llvm(), {0u, 0u}));
				llvm::Value* intVal = nullptr;
				if (val->is_ghost_ref() || val->is_ref()) {
					if (val->is_ref()) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					intVal = ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), val->get_llvm());
				} else {
					intVal = val->get_llvm();
				}
				if (intTy->get_bitwidth() < 64u) {
					intVal = ctx->irCtx->builder.CreateIntCast(intVal, llvm::Type::getInt64Ty(ctx->irCtx->llctx), true);
				}
				formatString += "%i";
				printVals.push_back(intVal);
			}
		} else if (valTy->is_unsigned() ||
		           (valTy->is_native_type() && valTy->as_native_type()->get_subtype()->is_unsigned())) {
			if (val->is_prerun_value()) {
				auto valStr = valTy->to_prerun_generic_string(val->as_prerun()).value();
				formatString += valStr;
			} else {
				auto uintTy =
				    valTy->is_unsigned() ? valTy->as_unsigned() : valTy->as_native_type()->get_subtype()->as_unsigned();
				// auto strVal =
				//     int_to_std_string(false, ctx, ir::Value::get(val->get_llvm(), uintTy, false),
				//     valRange);
				// formatString += "%.*s";
				// printVals.push_back(ctx->irCtx->builder.CreateExtractValue(strVal->get_llvm(), {1u}));
				// printVals.push_back(ctx->irCtx->builder.CreateExtractValue(strVal->get_llvm(), {0u, 0u}));
				llvm::Value* intVal = nullptr;
				if (val->is_ghost_ref() || val->is_ref()) {
					if (val->is_ref()) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					intVal = ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), val->get_llvm());
				} else {
					intVal = val->get_llvm();
				}
				if (uintTy->get_bitwidth() < 64u) {
					intVal =
					    ctx->irCtx->builder.CreateIntCast(intVal, llvm::Type::getInt64Ty(ctx->irCtx->llctx), false);
				}
				formatString += "%u";
				printVals.push_back(intVal);
			}
		} else if (valTy->is_float() ||
		           (valTy->is_native_type() && valTy->as_native_type()->get_subtype()->is_float())) {
			if (val->is_prerun_value()) {
				auto valStr = valTy->to_prerun_generic_string(val->as_prerun()).value();
				formatString += valStr;
			} else {
				auto floatTy =
				    valTy->is_float() ? valTy->as_float() : valTy->as_native_type()->get_subtype()->as_float();
				llvm::Value* floatVal = nullptr;
				if (val->is_ghost_ref() || val->is_ref()) {
					if (val->is_ref()) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					floatVal = ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), val->get_llvm());
				} else {
					floatVal = val->get_llvm();
				}
				if (floatTy->get_float_kind() != ir::FloatTypeKind::_64) {
					floatVal = ctx->irCtx->builder.CreateFPCast(floatVal, llvm::Type::getInt64Ty(ctx->irCtx->llctx));
				}
				formatString += "%u";
				printVals.push_back(floatVal);
			}
		} else if (valTy->is_mark() || (valTy->is_native_type() && valTy->as_native_type()->get_subtype()->is_mark())) {
			if (val->is_prerun_value()) {
				auto valStr = valTy->to_prerun_generic_string(val->as_prerun()).value();
				formatString += valStr;
			} else {
				if (valTy->as_mark()->is_slice()) {
					if (val->is_ref() || val->is_ghost_ref()) {
						if (val->is_ref()) {
							val->load_ghost_ref(ctx->irCtx->builder);
						}
						val = ir::Value::get(
						    ctx->irCtx->builder.CreateLoad(
						        valTy->as_mark()->get_subtype()->get_llvm_type()->getPointerTo(
						            ctx->irCtx->dataLayout.getProgramAddressSpace()),
						        ctx->irCtx->builder.CreateStructGEP(valTy->get_llvm_type(), val->get_llvm(), 0u)),
						    ir::MarkType::get(false, valTy->as_mark()->get_subtype(), false, MarkOwner::of_anonymous(),
						                      false, ctx->irCtx),
						    false);
					} else {
						val = ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}),
						                     ir::MarkType::get(false, valTy->as_mark()->get_subtype(), false,
						                                       MarkOwner::of_anonymous(), false, ctx->irCtx),
						                     false);
					}
				} else {
					if (val->is_ref() || val->is_ghost_ref()) {
						val->load_ghost_ref(ctx->irCtx->builder);
						if (val->is_ref()) {
							val = ir::Value::get(
							    ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), val->get_llvm()), valTy, false);
						}
					}
				}
				formatString += "%p";
				printVals.push_back(val->get_llvm());
			}
		} else if (valTy->is_tuple()) {
			// FIXME - Update when named tuple members are allowed
			formatString += "(";
			auto subTypes = valTy->as_tuple()->getSubTypes();
			if (val->is_ref() || val->is_ghost_ref()) {
				val->load_ghost_ref(ctx->irCtx->builder);
			}
			for (usize i = 0; i < subTypes.size(); i++) {
				auto* subVal = val->get_llvm();
				subVal =
				    (val->is_ref() || val->is_ghost_ref())
				        ? ctx->irCtx->builder.CreateStructGEP(subTypes.at(i)->get_llvm_type(), subVal, i)
				        : (val->is_prerun_value() ? val->get_llvm_constant()->getAggregateElement(i)
				                                  : ctx->irCtx->builder.CreateExtractValue(subVal, {(unsigned int)i}));
				if (subTypes.at(i)->is_ref() && (val->is_ref() || val->is_ghost_ref())) {
					subVal = ctx->irCtx->builder.CreateLoad(subTypes.at(i)->get_llvm_type(), subVal);
				}
				formatValue(ir::Value::get(subVal,
				                           (val->is_ref() || val->is_ghost_ref())
				                               ? ir::RefType::get(val->is_ref()
				                                                      ? val->get_ir_type()->as_ref()->has_variability()
				                                                      : val->is_variable(),
				                                                  subTypes.at(i), ctx->irCtx)
				                               : subTypes.at(i),
				                           val->is_variable()),
				            valRange);
				if (i != (subTypes.size() - 1)) {
					formatString += "; ";
				}
			}
			formatString += ")";
			if (val->is_value()) {
				ctx->irCtx->builder.CreateStore(
				    val->get_llvm(), ctx->get_fn()
				                         ->get_block()
				                         ->new_local(ctx->get_fn()->get_random_alloca_name(), valTy, true, valRange)
				                         ->get_llvm());
			}
		} else if (valTy->is_array()) {
			formatString += "[";
			auto* subType = valTy->as_array()->get_element_type();
			if (valTy->as_array()->get_length() > 0) {
				for (usize i = 0; i < valTy->as_array()->get_length(); i++) {
					auto* subVal = (val->is_ref() || val->is_ghost_ref())
					                   ? ctx->irCtx->builder.CreateInBoundsGEP(
					                         valTy->get_llvm_type(), val->get_llvm(),
					                         // TODO - Change index type to usize llvm equivalent
					                         {llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), 0u),
					                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), i)})
					                   : (val->is_prerun_value()
					                          ? val->get_llvm_constant()->getAggregateElement(i)
					                          : ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {(u32)i}));
					if (subType->is_ref() && (val->is_ref() || val->is_ghost_ref())) {
						subVal = ctx->irCtx->builder.CreateLoad(subType->get_llvm_type(), subVal);
					}
					formatValue(ir::Value::get(subVal,
					                           (val->is_ref() || val->is_ghost_ref())
					                               ? (ir::RefType::get(
					                                     val->is_ref() ? val->get_ir_type()->as_ref()->has_variability()
					                                                   : val->is_variable(),
					                                     subType, ctx->irCtx))
					                               : subType,
					                           val->is_variable()),
					            valRange);
					if (i != (valTy->as_array()->get_length() - 1)) {
						formatString += ", ";
					}
				}
			}
			formatString += "]";
		} else {
			if ((not val->is_prerun_value()) && valTy->is_expanded() && ir::StdLib::is_std_lib_found() &&
			    ir::StdLib::has_string_type() &&
			    valTy->as_expanded()->has_to_convertor(ir::StdLib::get_string_type())) {
				auto stringTy = ir::StdLib::get_string_type();
				auto eTy      = valTy->as_expanded();
				auto toFn     = eTy->get_to_convertor(stringTy);
				if (not val->is_ref() && not val->is_ghost_ref()) {
					auto candVal = ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), valTy,
					                                                     true, valRange);
					ctx->irCtx->builder.CreateStore(val->get_llvm(), candVal->get_llvm());
					val = candVal;
				} else if (val->is_ref()) {
					val->load_ghost_ref(ctx->irCtx->builder);
				}
				auto stringVal = toFn->call(ctx->irCtx, {val->get_llvm()}, None, ctx->mod);
				formatString += "%.*s";
				printVals.push_back(ctx->irCtx->builder.CreateExtractValue(stringVal->get_llvm(), {1u}));
				printVals.push_back(ctx->irCtx->builder.CreateExtractValue(stringVal->get_llvm(), {0u, 0u}));
				(void)ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), stringTy, false,
				                                            valRange);
			} else if (not val->is_prerun_value() && ir::StdLib::is_std_lib_found() && ir::StdLib::has_string_type() &&
			           valTy->is_same(ir::StdLib::get_string_type())) {
				auto stringTy = ir::StdLib::get_string_type();
				if (val->is_ref() || val->is_ghost_ref()) {
					if (val->is_ref()) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					formatString += "%.*s";
					printVals.push_back(ctx->irCtx->builder.CreateLoad(
					    ir::NativeType::get_usize(ctx->irCtx)->get_llvm_type(),
					    ctx->irCtx->builder.CreateStructGEP(stringTy->get_llvm_type(), val->get_llvm(), 1u)));
					printVals.push_back(ctx->irCtx->builder.CreateLoad(
					    llvm::Type::getInt8Ty(ctx->irCtx->llctx)
					        ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
					    ctx->irCtx->builder.CreateStructGEP(
					        stringTy->get_llvm_type()->getStructElementType(0u),
					        ctx->irCtx->builder.CreateStructGEP(stringTy->get_llvm_type(), val->get_llvm(), 0u), 0u)));
				} else {
					formatString += "%.*s";
					printVals.push_back(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {1u}));
					printVals.push_back(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u, 0u}));
					ctx->irCtx->builder.CreateStore(val->get_llvm(),
					                                ctx->get_fn()
					                                    ->get_block()
					                                    ->new_local(ctx->get_fn()->get_random_alloca_name(),
					                                                ir::StdLib::get_string_type(), true, valRange)
					                                    ->get_llvm());
				}
			} else {
				ctx->Error("Cannot format expression of type " + ctx->color(valTy->to_string()), valRange);
			}
		}
	};
	for (usize i = 0; i < values.size(); i++) {
		auto val = values.at(i);
		formatValue(val, i < ranges.size() ? ranges[i] : fileRange);
	}
	return {formatString, printVals};
}

void Logic::exit_thread(ir::Function* fun, ast::EmitCtx* ctx, FileRange rangeVal) {
	auto triple = ctx->irCtx->clangTargetInfo->getTriple();
	if (triple.isWindowsMSVCEnvironment()) {
		auto exitFnName =
		    ctx->mod->link_internal_dependency(InternalDependency::windowsExitThread, ctx->irCtx, rangeVal);
		auto exitThreadPtr  = ctx->mod->get_llvm_module()->getGlobalVariable(exitFnName);
		auto exitThreadFnTy = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx->irCtx->llctx),
		                                              {llvm::Type::getInt32Ty(ctx->irCtx->llctx)}, false);
		ctx->irCtx->builder.CreateCall(
		    exitThreadFnTy, ctx->irCtx->builder.CreateLoad(llvm::PointerType::get(exitThreadFnTy, 0u), exitThreadPtr),
		    {llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), 1u)});
	} else {
		auto exitFnName    = ctx->mod->link_internal_dependency(InternalDependency::pthreadExit, ctx->irCtx, rangeVal);
		auto pthreadExitFn = ctx->mod->get_llvm_module()->getFunction(exitFnName);
		ctx->irCtx->builder.CreateCall(
		    pthreadExitFn->getFunctionType(), pthreadExitFn,
		    {llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx)
		                                        ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()))});
	}
}

void Logic::exit_program(ir::Function* fun, ast::EmitCtx* ctx, FileRange rangeVal) {
	auto exitFnName = ctx->mod->link_internal_dependency(InternalDependency::exitProgram, ctx->irCtx, rangeVal);
	auto exitFun    = ctx->mod->get_llvm_module()->getFunction(exitFnName);
	ctx->irCtx->builder.CreateCall(exitFun->getFunctionType(), exitFun,
	                               {llvm::ConstantInt::get(NativeType::get_int(ctx->irCtx)->get_llvm_type(), 1u)});
}

void Logic::panic_in_function(ir::Function* fun, Vec<ir::Value*> values, Vec<FileRange> ranges, FileRange fileRange,
                              ast::EmitCtx* ctx) {
	fileRange.file     = fs::absolute(fileRange.file);
	auto  startMessage = ir::TextType::create_value(ctx->irCtx, ctx->mod,
	                                                "\nFunction " + fun->get_full_name() + " panicked at " +
	                                                    fileRange.start_to_string() + " => ");
	auto* mod          = fun->get_module();
	auto  printfName   = mod->link_internal_dependency(InternalDependency::printf, ctx->irCtx, fileRange);
	auto  printFn      = mod->get_llvm_module()->getFunction(printfName);
	auto  formatRes    = format_values(ctx, values, ranges, fileRange);

	Vec<llvm::Value*> printVals{ctx->irCtx->builder.CreateGlobalStringPtr(
	                                "%.*s" + formatRes.first + "\n\n", ctx->irCtx->get_global_string_name(),
	                                ctx->irCtx->dataLayout.getDefaultGlobalsAddressSpace()),
	                            startMessage->get_llvm_constant()->getAggregateElement(1u),
	                            startMessage->get_llvm_constant()->getAggregateElement(0u)};
	for (auto* val : formatRes.second) {
		printVals.push_back(val);
	}
	ctx->irCtx->builder.CreateCall(printFn->getFunctionType(), printFn, printVals);
	auto cfg = cli::Config::get();
	if (cfg->is_freestanding()) {
		switch (cfg->get_panic_strategy()) {
			case cli::PanicStrategy::exitThread: {
				exit_thread(fun, ctx, fileRange);
				break;
			}
			case cli::PanicStrategy::exitProgram: {
				exit_program(fun, ctx, fileRange);
				break;
			}
			case cli::PanicStrategy::handler: {
				auto handlerName = fun->get_module()->link_internal_dependency(InternalDependency::panicHandler,
				                                                               ctx->irCtx, fileRange);
				auto handlerFn   = mod->get_llvm_module()->getFunction(handlerName);
				ctx->irCtx->builder.CreateCall(handlerFn->getFunctionType(), handlerFn, printVals);
				break;
			}
			default:
				break;
		}
	} else if (not cfg->has_panic_strategy()) {
		exit_thread(fun, ctx, fileRange);
	} else {
		switch (cfg->get_panic_strategy()) {
			case cli::PanicStrategy::exitThread: {
				exit_thread(fun, ctx, fileRange);
				break;
			}
			case cli::PanicStrategy::exitProgram: {
				exit_program(fun, ctx, fileRange);
				break;
			}
			case cli::PanicStrategy::handler: {
				auto handlerName = fun->get_module()->link_internal_dependency(InternalDependency::panicHandler,
				                                                               ctx->irCtx, fileRange);
				auto handlerFn   = mod->get_llvm_module()->getFunction(handlerName);
				ctx->irCtx->builder.CreateCall(handlerFn->getFunctionType(), handlerFn, printVals);
				break;
			}
			default:
				break;
		}
	}
}

String Logic::get_generic_variant_name(String mainName, Vec<ir::GenericToFill*>& fills) {
	String result(std::move(mainName));
	result.append(":[");
	SHOW("Initial part of name")
	for (usize i = 0; i < fills.size(); i++) {
		result += fills.at(i)->to_string();
		if (i != (fills.size() - 1)) {
			result.append(", ");
		}
	}
	SHOW("Middle part done")
	result.append("]");
	return result;
}

llvm::AllocaInst* Logic::newAlloca(ir::Function* fun, Maybe<String> name, llvm::Type* type) {
	llvm::AllocaInst* result = nullptr;
	llvm::Function*   func   = fun->get_llvm_function();
	if (func->getEntryBlock().empty()) {
		result = new llvm::AllocaInst(type, 0U, name.value_or(fun->get_random_alloca_name()), &func->getEntryBlock());
	} else {
		llvm::Instruction* inst = nullptr;
		// NOLINTNEXTLINE(modernize-loop-convert)
		for (auto instr = func->getEntryBlock().begin(); instr != func->getEntryBlock().end(); instr++) {
			if (llvm::isa<llvm::AllocaInst>(&*instr)) {
				continue;
			} else {
				inst = &*instr;
				break;
			}
		}
		if (inst) {
			result = new llvm::AllocaInst(type, 0U, name.value_or(fun->get_random_alloca_name()), inst);
		} else {
			result =
			    new llvm::AllocaInst(type, 0U, name.value_or(fun->get_random_alloca_name()), &func->getEntryBlock());
		}
	}
	return result;
}

bool Logic::compare_prerun_text(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
                                llvm::Constant* rhsCount, llvm::LLVMContext& llCtx) {
	if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) ==
	    (*llvm::cast<llvm::ConstantInt>(rhsCount)->getValue().getRawData())) {
		if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) == 0u) {
			return true;
		}
		return llvm::cast<llvm::ConstantDataArray>(lhsBuff->getOperand(0u))->getAsString() ==
		       llvm::cast<llvm::ConstantDataArray>(rhsBuff->getOperand(0u))->getAsString();
	} else {
		return false;
	}
}

useit ir::Value* Logic::compare_text(bool isEquality, ir::Value* lhsEmit, ir::Value* rhsEmit, FileRange lhsRange,
                                     FileRange rhsRange, FileRange fileRange, ast::EmitCtx* ctx) {
	llvm::Value *lhsBuff, *lhsCount, *rhsBuff, *rhsCount;
	bool         isConstantLHS = false, isConstantRHS = false;
	auto*        int64Type  = llvm::Type::getInt64Ty(ctx->irCtx->llctx);
	auto         textTy     = ir::TextType::get(ctx->irCtx);
	auto*        boolType   = llvm::Type::getInt1Ty(ctx->irCtx->llctx);
	auto*        int8Type   = llvm::Type::getInt8Ty(ctx->irCtx->llctx);
	auto*        boolIRType = ir::UnsignedType::create_bool(ctx->irCtx);
	if (lhsEmit->is_llvm_constant()) {
		lhsBuff       = lhsEmit->get_llvm_constant()->getAggregateElement(0u);
		lhsCount      = lhsEmit->get_llvm_constant()->getAggregateElement(1u);
		isConstantLHS = true;
	} else {
		if (lhsEmit->get_ir_type()->is_ref()) {
			lhsEmit->load_ghost_ref(ctx->irCtx->builder);
		} else if (not lhsEmit->is_ghost_ref()) {
			lhsEmit = lhsEmit->make_local(ctx, None, lhsRange);
		}
		lhsBuff = ctx->irCtx->builder.CreateLoad(
		    int8Type->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
		    ctx->irCtx->builder.CreateStructGEP(textTy->get_llvm_type(), lhsEmit->get_llvm(), 0u));
		lhsCount = ctx->irCtx->builder.CreateLoad(
		    int64Type, ctx->irCtx->builder.CreateStructGEP(textTy->get_llvm_type(), lhsEmit->get_llvm(), 1u));
	}
	if (rhsEmit->is_llvm_constant()) {
		rhsBuff       = rhsEmit->get_llvm_constant()->getAggregateElement(0u);
		rhsCount      = rhsEmit->get_llvm_constant()->getAggregateElement(1u);
		isConstantRHS = true;
	} else {
		if (rhsEmit->get_ir_type()->is_ref()) {
			rhsEmit->load_ghost_ref(ctx->irCtx->builder);
		} else if (not rhsEmit->is_ghost_ref()) {
			rhsEmit = rhsEmit->make_local(ctx, None, rhsRange);
		}
		rhsBuff = ctx->irCtx->builder.CreateLoad(
		    int8Type->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
		    ctx->irCtx->builder.CreateStructGEP(textTy->get_llvm_type(), rhsEmit->get_llvm(), 0u));
		rhsCount = ctx->irCtx->builder.CreateLoad(
		    int64Type, ctx->irCtx->builder.CreateStructGEP(textTy->get_llvm_type(), rhsEmit->get_llvm(), 1u));
	}
	if (isConstantLHS && isConstantRHS) {
		auto strCmpRes = ir::Logic::compare_prerun_text(
		    llvm::cast<llvm::Constant>(lhsBuff), llvm::cast<llvm::Constant>(lhsCount),
		    llvm::cast<llvm::Constant>(rhsBuff), llvm::cast<llvm::Constant>(rhsCount), ctx->irCtx->llctx);
		return ir::PrerunValue::get(llvm::ConstantInt::get(boolType, isEquality ? strCmpRes : not strCmpRes),
		                            boolIRType)
		    ->with_range(fileRange);
	}
	auto* curr              = ctx->get_fn()->get_block();
	auto* lenCheckTrueBlock = ir::Block::create(ctx->get_fn(), curr);
	auto* strCmpTrueBlock   = ir::Block::create(ctx->get_fn(), curr);
	auto* strCmpFalseBlock  = ir::Block::create(ctx->get_fn(), curr);
	auto* iterCondBlock     = ir::Block::create(ctx->get_fn(), lenCheckTrueBlock);
	auto* iterTrueBlock     = ir::Block::create(ctx->get_fn(), lenCheckTrueBlock);
	auto* iterIncrBlock     = ir::Block::create(ctx->get_fn(), iterTrueBlock);
	auto* iterFalseBlock    = ir::Block::create(ctx->get_fn(), lenCheckTrueBlock);
	auto* restBlock         = ir::Block::create(ctx->get_fn(), curr->get_parent());
	restBlock->link_previous_block(curr);
	auto* qatStrCmpIndex = ctx->get_fn()->get_str_comparison_index();
	// NOTE - Length equality check
	ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpEQ(lhsCount, rhsCount), lenCheckTrueBlock->get_bb(),
	                                 strCmpFalseBlock->get_bb());
	//
	// NOTE - Length matches
	lenCheckTrueBlock->set_active(ctx->irCtx->builder);
	ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(int64Type, 0u), qatStrCmpIndex->get_llvm());
	(void)ir::add_branch(ctx->irCtx->builder, iterCondBlock->get_bb());
	//
	// NOTE - Checking the iteration count
	iterCondBlock->set_active(ctx->irCtx->builder);
	ctx->irCtx->builder.CreateCondBr(
	    ctx->irCtx->builder.CreateICmpULT(ctx->irCtx->builder.CreateLoad(int64Type, qatStrCmpIndex->get_llvm()),
	                                      lhsCount),
	    iterTrueBlock->get_bb(), iterFalseBlock->get_bb());
	//
	// NOTE - Iteration check is true
	iterTrueBlock->set_active(ctx->irCtx->builder);
	ctx->irCtx->builder.CreateCondBr(
	    ctx->irCtx->builder.CreateICmpEQ(
	        ctx->irCtx->builder.CreateLoad(
	            int8Type,
	            ctx->irCtx->builder.CreateInBoundsGEP(
	                int8Type, lhsBuff, {ctx->irCtx->builder.CreateLoad(int64Type, qatStrCmpIndex->get_llvm())})),
	        ctx->irCtx->builder.CreateLoad(
	            int8Type,
	            ctx->irCtx->builder.CreateInBoundsGEP(
	                int8Type, rhsBuff, {ctx->irCtx->builder.CreateLoad(int64Type, qatStrCmpIndex->get_llvm())}))),
	    iterIncrBlock->get_bb(), iterFalseBlock->get_bb());
	//
	// NOTE - Increment string slice iteration count
	iterIncrBlock->set_active(ctx->irCtx->builder);
	ctx->irCtx->builder.CreateStore(
	    ctx->irCtx->builder.CreateAdd(ctx->irCtx->builder.CreateLoad(int64Type, qatStrCmpIndex->get_llvm()),
	                                  llvm::ConstantInt::get(int64Type, 1u)),
	    qatStrCmpIndex->get_llvm());
	(void)ir::add_branch(ctx->irCtx->builder, iterCondBlock->get_bb());
	//
	// NOTE - Iteration check is false, time to check if the count matches
	iterFalseBlock->set_active(ctx->irCtx->builder);
	ctx->irCtx->builder.CreateCondBr(
	    ctx->irCtx->builder.CreateICmpEQ(lhsCount,
	                                     ctx->irCtx->builder.CreateLoad(int64Type, qatStrCmpIndex->get_llvm())),
	    strCmpTrueBlock->get_bb(), strCmpFalseBlock->get_bb());
	//
	// NOTE - Merging branches to the resume block
	strCmpTrueBlock->set_active(ctx->irCtx->builder);
	(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
	strCmpFalseBlock->set_active(ctx->irCtx->builder);
	(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
	//
	// NOTE - Control flow resumes
	restBlock->set_active(ctx->irCtx->builder);
	auto* strCmpRes = ctx->irCtx->builder.CreatePHI(boolType, 2);
	strCmpRes->addIncoming(llvm::ConstantInt::get(boolType, isEquality ? 1u : 0u), strCmpTrueBlock->get_bb());
	strCmpRes->addIncoming(llvm::ConstantInt::get(boolType, isEquality ? 0u : 1u), strCmpFalseBlock->get_bb());
	return ir::Value::get(strCmpRes, boolIRType, false)->with_range(fileRange);
}

} // namespace qat::ir
