#include "./function.hpp"
#include "../IR/qat_module.hpp"
#include "../IR/types/void.hpp"
#include "../show.hpp"
#include "./emit_ctx.hpp"
#include "./sentence.hpp"
#include "./types/generic_abstract.hpp"
#include "./types/prerun_generic.hpp"
#include "./types/typed_generic.hpp"
#include "internal.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>

namespace qat::ast {

FunctionPrototype::~FunctionPrototype() {
	for (auto* arg : arguments) {
		std::destroy_at(arg);
	}
}

bool FunctionPrototype::is_generic() const { return not generics.empty(); }

Vec<GenericAbstractType*> FunctionPrototype::get_generics() const { return generics; }

void FunctionPrototype::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	mod->entity_name_check(irCtx, name, is_generic() ? ir::EntityType::genericFunction : ir::EntityType::function);
	entityState =
	    mod->add_entity(name, is_generic() ? ir::EntityType::genericFunction : ir::EntityType::function, this,
	                    is_generic() ? ir::EmitPhase::phase_2
	                                 : (definition.has_value() ? ir::EmitPhase::phase_3 : ir::EmitPhase::phase_2));
}

void FunctionPrototype::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (defineChecker != nullptr) {
		defineChecker->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	if (returnType.has_value()) {
		returnType.value()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
	}
	if (is_generic()) {
		for (auto gen : generics) {
			if (gen->is_prerun() && gen->as_prerun()->hasDefault()) {
				gen->as_prerun()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
				                                                       entityState, ctx);
			} else if (gen->is_typed() && gen->as_typed()->hasDefault()) {
				gen->as_typed()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
				                                                      entityState, ctx);
			}
		}
	}
	for (auto arg : arguments) {
		if (arg->get_type()) {
			arg->get_type()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		}
	}
	if (definition.has_value()) {
		for (auto snt : definition.value().first) {
			snt->update_dependencies(is_generic() ? ir::EmitPhase::phase_2 : ir::EmitPhase::phase_3,
			                         ir::DependType::complete, entityState, ctx);
		}
	}
}

void FunctionPrototype::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	if ((phase == ir::EmitPhase::phase_1) && (defineChecker != nullptr)) {
		auto cond = defineChecker->emit(emitCtx);
		if (not cond->get_ir_type()->is_bool()) {
			irCtx->Error("The condition for defining a function should be of " + irCtx->color("bool") + " type",
			             defineChecker->fileRange);
		}
		if (not llvm::cast<llvm::ConstantInt>(cond->get_llvm_constant())->getValue().getBoolValue()) {
			entityState->complete_manually();
			return;
		}
	}
	if (is_generic()) {
		if (phase == ir::EmitPhase::phase_2) {
			for (auto* gen : generics) {
				gen->emit(EmitCtx::get(irCtx, mod));
			}
			genericFn = ir::GenericFunction::create(name, generics, genericConstraint, this, mod,
			                                        emitCtx->get_visibility_info(visibSpec));
		}
	} else {
		if (phase == ir::EmitPhase::phase_2) {
			function = create_function(mod, irCtx);
		} else if (phase == ir::EmitPhase::phase_3) {
			emit_definition(mod, irCtx);
		}
	}
}

ir::Function* FunctionPrototype::create_function(ir::Mod* mod, ir::Ctx* irCtx) const {
	SHOW("Creating function " << name.value << " with generic count: " << generics.size())
	auto emitCtx = EmitCtx::get(irCtx, mod);
	emitCtx->name_check_in_module(name, is_generic() ? "generic function" : "function",
	                              is_generic() ? Maybe<u64>(genericFn->get_id()) : None);
	Vec<ir::Type*> generatedTypes;
	String         fnName = name.value;
	SHOW("Creating function")
	if (genericFn) {
		fnName = variantName.value();
	}
	if ((fnName == "main") && (mod->get_full_name() == "")) {
		SHOW("Is main function")
		linkageType = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage;
		isMainFn    = true;
		if (not returnType.has_value()) {
			irCtx->Error(
			    "The " + irCtx->color("main") + " function is required to always give a value of type " +
			        irCtx->color("int") +
			        " to indicate the resultant status of the program to the operating system. Give a " +
			        irCtx->color("0") +
			        " value at the end of the main function to indicate success, if you don't care about the status of the program for now",
			    fileRange);
		}
	} else if (fnName == "main") {
		irCtx->Error("Main function cannot be inside a named module", name.range);
	}
	SHOW("Generating types")
	bool isVariadic = false;
	for (usize i = 0; i < arguments.size(); i++) {
		auto arg = arguments[i];
		if (arg->is_member_arg()) {
			irCtx->Error("Function is not a member function of a core type and cannot "
			             "use member argument syntax",
			             arg->get_name().range);
		}
		if (arg->is_variadic_arg()) {
			isVariadic = true;
			if (i != (arguments.size() - 1)) {
				irCtx->Error("Variadic argument should always be the last argument", arg->get_name().range);
			}
		} else {
			auto* genType = arg->get_type()->emit(EmitCtx::get(irCtx, mod));
			generatedTypes.push_back(genType);
		}
	}
	SHOW("Types generated")
	if (isMainFn) {
		if (visibSpec.has_value() && visibSpec->kind != VisibilityKind::pub) {
			irCtx->Error("This is the " + irCtx->color("main") + " function, please make this function public like " +
			                 irCtx->color("pub main"),
			             name.range);
		}
		if (mod->has_main_function()) {
			irCtx->Error(irCtx->color("main") + " function already exists in this module. Please check "
			                                    "the codebase",
			             fileRange);
		} else {
			if (generatedTypes.size() == 1) {
				if (generatedTypes.at(0)->is_mark() && generatedTypes.at(0)->as_mark()->is_slice() &&
				    generatedTypes.at(0)->as_mark()->get_owner().is_of_anonymous()) {
					if (generatedTypes.at(0)->as_mark()->is_subtype_variable()) {
						irCtx->Error("Type of the argument of the " + irCtx->color("main") +
						                 " function, cannot be a slice with variability. It should be of type " +
						                 irCtx->color("slice![cstring]"),
						             arguments[0]->get_type()->fileRange);
					}
					mod->set_has_main_function();
					irCtx->hasMain = true;
				} else {
					irCtx->Error("Type of the argument of the " + irCtx->color("main") + " function should be " +
					                 irCtx->color("slice![cstring]"),
					             arguments[0]->get_type()->fileRange);
				}
			} else if (generatedTypes.empty()) {
				mod->set_has_main_function();
				irCtx->hasMain = true;
			} else {
				irCtx->Error("Main function can either have one argument, or none at "
				             "all. Please check the codebase and make necessary changes",
				             fileRange);
			}
		}
	}
	Vec<ir::Argument> args;
	SHOW("Setting variability of arguments")
	if (isMainFn) {
		if (not arguments.empty()) {
			args.push_back(
			    // NOLINTNEXTLINE(readability-magic-numbers)
			    ir::Argument::Create(
			        Identifier(arguments.at(0)->get_name().value + "'count", arguments.at(0)->get_name().range),
			        ir::UnsignedType::create(32u, irCtx), 0u));
			args.push_back(ir::Argument::Create(
			    Identifier(arguments.at(0)->get_name().value + "'data", arguments.at(0)->get_name().range),
			    ir::MarkType::get(false, ir::NativeType::get_cstr(irCtx), true, ir::MarkOwner::of_anonymous(), false,
			                      irCtx),
			    1u));
		}
	} else {
		for (usize i = 0; i < generatedTypes.size(); i++) {
			args.push_back(
			    arguments.at(i)->is_variable()
			        ? ir::Argument::CreateVariable(arguments.at(i)->get_name(),
			                                       arguments.at(i)->get_type()->emit(EmitCtx::get(irCtx, mod)), i)
			        : ir::Argument::Create(arguments.at(i)->get_name(), generatedTypes.at(i), i));
		}
		if (isVariadic) {
			args.push_back(ir::Argument::CreateVariadic("", arguments.back()->get_name().range, arguments.size() - 1));
		}
	}
	SHOW("Variability setting complete")
	auto* retTy = returnType.has_value() ? returnType.value()->emit(emitCtx) : ir::VoidType::get(irCtx->llctx);
	if (isMainFn) {
		if (not(retTy->is_native_type() && retTy->as_native_type()->get_c_type_kind() == ir::NativeTypeKind::Int)) {
			irCtx->Error(irCtx->color("main") + " function expects to have a given type of " + irCtx->color("int"),
			             fileRange);
		}
	}
	Maybe<ir::MetaInfo> irMetaInfo;
	bool                inlineFunction = false;
	if (metaInfo.has_value()) {
		metaInfo.value().set_parent_as_function();
		irMetaInfo = metaInfo.value().toIR(emitCtx);
		if (irMetaInfo.value().has_key(ir::MetaInfo::inlineKey)) {
			inlineFunction = irMetaInfo.value().get_value_as_bool(ir::MetaInfo::inlineKey).value();
		}
	}
	if (is_generic()) {
		Vec<ir::GenericArgument*> genericTypes;
		for (auto* gen : generics) {
			genericTypes.push_back(gen->toIRGenericType());
		}
		SHOW("About to create generic function")
		auto* fun = ir::Function::Create(mod, Identifier(fnName, name.range), None, std::move(genericTypes),
		                                 inlineFunction, ir::ReturnType::get(retTy), args, fileRange,
		                                 emitCtx->get_visibility_info(visibSpec), irCtx, None, irMetaInfo);
		SHOW("Created IR function")
		return fun;
	} else {
		SHOW("About to create function")
		auto* fun = ir::Function::Create(
		    mod, Identifier(fnName, name.range),
		    isMainFn ? Maybe<LinkNames>(
		                   LinkNames({LinkNameUnit(irCtx->clangTargetInfo->getTriple().isWasm() ? "_start" : "main",
		                                           LinkUnitType::function)},
		                             "C", nullptr))
		             : None,
		    {}, inlineFunction, ir::ReturnType::get(retTy), args, fileRange, emitCtx->get_visibility_info(visibSpec),
		    irCtx,
		    definition.has_value()
		        ? None
		        : Maybe<llvm::GlobalValue::LinkageTypes>(llvm::GlobalValue::LinkageTypes::ExternalLinkage),
		    irMetaInfo);
		SHOW("Created IR function")
		if (irMetaInfo.has_value() && irMetaInfo->has_key(ir::MetaInfo::providesKey)) {
			auto proVal = irMetaInfo->get_value_as_string_for(ir::MetaInfo::providesKey);
			if (proVal.has_value()) {
				auto inDep = ir::internal_dependency_from_string(proVal.value());
				if (not inDep.has_value()) {
					irCtx->Error("The " + irCtx->color(ir::MetaInfo::providesKey) + " key has a value of " +
					                 irCtx->color(proVal.value()) + ", which is not a recognised internal dependency",
					             irMetaInfo->get_value_range_for(ir::MetaInfo::providesKey));
				}
				if (ir::Mod::has_provided_function(inDep.value())) {
					auto fn       = ir::Mod::get_provided_function(inDep.value());
					auto depRange = fn->has_definition_range() ? fn->get_definition_range() : fn->get_name().range;
					irCtx->Error("The internal dependency " + irCtx->color(proVal.value()) +
					                 " has already been provided",
					             irMetaInfo->get_value_range_for(ir::MetaInfo::providesKey),
					             Pair<String, FileRange>{"The previously provided entity can be found here", depRange});
				}
				switch (inDep.value()) {
					case ir::InternalDependency::printf: {
						auto printfSig = ir::Internal::printf_signature(irCtx);
						if (not fun->get_ir_type()->is_same(printfSig)) {
							irCtx->Error(
							    "The internal dependency " +
							        irCtx->color(ir::internal_dependency_to_string(inDep.value())) +
							        " provided by this function has an unexpected signature. The expected signature is " +
							        irCtx->color(printfSig->to_string()) + " but the signature of this function is " +
							        irCtx->color(fun->get_ir_type()->to_string()),
							    fileRange);
						}
						break;
					}
					case ir::InternalDependency::malloc: {
						auto mallocSig = ir::Internal::malloc_signature(irCtx);
						if (not fun->get_ir_type()->is_same(mallocSig)) {
							irCtx->Error(
							    "The internal dependency " +
							        irCtx->color(ir::internal_dependency_to_string(inDep.value())) +
							        " provided by this function has an unexpected signature. The expected signature is " +
							        irCtx->color(mallocSig->to_string()) + " but the signature of this function is " +
							        irCtx->color(fun->get_ir_type()->to_string()),
							    fileRange);
						}
						break;
					}
					case ir::InternalDependency::realloc: {
						auto reallocSig = ir::Internal::realloc_signature(irCtx);
						if (not fun->get_ir_type()->is_same(reallocSig)) {
							irCtx->Error(
							    "The internal dependency " +
							        irCtx->color(ir::internal_dependency_to_string(inDep.value())) +
							        " provided by this function has an unexpected signature. The expected signature is " +
							        irCtx->color(reallocSig->to_string()) + " but the signature of this function is " +
							        irCtx->color(fun->get_ir_type()->to_string()),
							    fileRange);
						}
						break;
					}
					case ir::InternalDependency::free: {
						auto freeSig = ir::Internal::free_signature(irCtx);
						if (not fun->get_ir_type()->is_same(freeSig)) {
							irCtx->Error(
							    "The internal dependency " +
							        irCtx->color(ir::internal_dependency_to_string(inDep.value())) +
							        " provided by this function has an unexpected signature. The expected signature is " +
							        irCtx->color(freeSig->to_string()) + " but the signature of this function is " +
							        irCtx->color(fun->get_ir_type()->to_string()),
							    fileRange);
						}
					}
					default: {
						irCtx->Error("The internal dependency " +
						                 irCtx->color(ir::internal_dependency_to_string(inDep.value())) +
						                 " cannot be provided at the moment",
						             fileRange);
					}
				}
				ir::Mod::add_provided_function(inDep.value(), fun);
			} else {
				irCtx->Error("Could not get the value of the " + irCtx->color(ir::MetaInfo::providesKey) +
				                 " key in the metadata of the function",
				             irMetaInfo->get_value_range_for(ir::MetaInfo::providesKey));
			}
		}
		return fun;
	}
}

void FunctionPrototype::emit_definition(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("Getting IR function from prototype for " << name.value)
	auto* fnEmit = function;
	SHOW("Set active function: " << fnEmit->get_full_name())
	auto* block = ir::Block::create(fnEmit, nullptr);
	SHOW("Created entry block")
	block->set_active(irCtx->builder);
	SHOW("Set new block as the active block")
	if (isMainFn) {
		SHOW("Calling module initialisers")
		auto modInits = ir::Mod::collect_mod_initialisers();
		for (auto* modFn : modInits) {
			(void)modFn->call(irCtx, {}, None, mod);
		}
		SHOW("Storing args for main function")
		if (fnEmit->get_ir_type()->as_function()->get_argument_count() == 2u) {
			auto* cmdArgsVal =
			    block->new_local(fnEmit->arg_name_at(0).value.substr(0, fnEmit->arg_name_at(0).value.find('\'')),
			                     ir::MarkType::get(false, ir::NativeType::get_cstr(irCtx), false,
			                                       ir::MarkOwner::of_anonymous(), true, irCtx),
			                     false, fnEmit->arg_name_at(0).range);
			SHOW("Storing argument pointer")
			irCtx->builder.CreateStore(
			    fnEmit->get_llvm_function()->getArg(1u),
			    irCtx->builder.CreateStructGEP(cmdArgsVal->get_ir_type()->get_llvm_type(), cmdArgsVal->get_llvm(), 0u));
			SHOW("Storing argument count")
			irCtx->builder.CreateStore(
			    irCtx->builder.CreateIntCast(fnEmit->get_llvm_function()->getArg(0u),
			                                 llvm::Type::getInt64Ty(irCtx->llctx), false),
			    irCtx->builder.CreateStructGEP(cmdArgsVal->get_ir_type()->get_llvm_type(), cmdArgsVal->get_llvm(), 1u));
		}
	} else {
		SHOW("About to allocate necessary arguments")
		auto argIRTypes = fnEmit->get_ir_type()->as_function()->get_argument_types();
		SHOW("Iteration run for function is: " << fnEmit->get_name().value)
		for (usize i = 0; i < argIRTypes.size(); i++) {
			auto argType = argIRTypes[i]->get_type();
			if (not argType->has_simple_copy() || argIRTypes[i]->is_variable()) {
				SHOW("Argument name is " << argIRTypes[i]->get_name())
				SHOW("Argument type is " << argType->to_string())
				auto* argVal = block->new_local(argIRTypes[i]->get_name(), argType, argIRTypes[i]->is_variable(),
				                                arguments[i]->get_name().range);
				SHOW("Created local value for the argument")
				irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), argVal->get_alloca(), false);
			}
		}
	}
	SHOW("Emitting sentences")
	emit_sentences(definition.value().first, EmitCtx::get(irCtx, mod)->with_function(fnEmit));
	SHOW("Sentences emitted")
	ir::function_return_handler(irCtx, fnEmit, fileRange);
	SHOW("Function return handler is complete")
}

void FunctionPrototype::set_variant_name(const String& value) const { variantName = value; }

void FunctionPrototype::unset_variant_name() const { variantName = None; }

Json FunctionPrototype::to_json() const {
	Vec<JsonValue> args;
	for (auto* arg : arguments) {
		auto aJson = Json()._("name", arg->get_name())._("type", arg->get_type() ? arg->get_type()->to_json() : Json());
		args.push_back(aJson);
	}
	Vec<JsonValue> sntcs;
	if (definition.has_value()) {
		for (auto* sentence : definition.value().first) {
			sntcs.push_back(sentence->to_json());
		}
	}
	return Json()
	    ._("nodeType", "functionPrototype")
	    ._("name", name)
	    ._("hasReturnType", returnType.has_value())
	    ._("returnType", returnType.has_value() ? returnType.value()->to_json() : JsonValue())
	    ._("arguments", args)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("hasMetaInfo", metaInfo.has_value())
	    ._("metaInfo", metaInfo.has_value() ? metaInfo.value().to_json() : JsonValue())
	    ._("sentences", sntcs)
	    ._("bodyRange", definition.has_value() ? definition.value().second : JsonValue());
}

} // namespace qat::ast
