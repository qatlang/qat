#include "./function.hpp"
#include "../IR/types/void.hpp"
#include "../show.hpp"
#include "./types/prerun_generic.hpp"
#include "./types/typed_generic.hpp"
#include "emit_ctx.hpp"
#include "sentence.hpp"
#include "types/generic_abstract.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionPrototype::~FunctionPrototype() {
  for (auto* arg : arguments) {
    std::destroy_at(arg);
  }
}

bool FunctionPrototype::isGeneric() const { return !generics.empty(); }

Vec<GenericAbstractType*> FunctionPrototype::getGenerics() const { return generics; }

void FunctionPrototype::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
  mod->entity_name_check(irCtx, name, isGeneric() ? ir::EntityType::genericFunction : ir::EntityType::function);
  entityState =
      mod->add_entity(name, isGeneric() ? ir::EntityType::genericFunction : ir::EntityType::function, this,
                      isGeneric() ? ir::EmitPhase::phase_1
                                  : (definition.has_value() ? ir::EmitPhase::phase_2 : ir::EmitPhase::phase_1));
}

void FunctionPrototype::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
  auto ctx = EmitCtx::get(irCtx, mod);
  if (returnType.has_value()) {
    returnType.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
  }
  if (isGeneric()) {
    for (auto gen : generics) {
      if (gen->is_prerun() && gen->as_prerun()->hasDefault()) {
        gen->as_prerun()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete,
                                                               entityState, ctx);
      } else if (gen->is_typed() && gen->as_typed()->hasDefault()) {
        gen->as_typed()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete,
                                                              entityState, ctx);
      }
    }
  }
  for (auto arg : arguments) {
    if (arg->get_type()) {
      arg->get_type()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
    }
  }
  if (definition.has_value()) {
    for (auto snt : definition.value().first) {
      snt->update_dependencies(isGeneric() ? ir::EmitPhase::phase_1 : ir::EmitPhase::phase_2, ir::DependType::complete,
                               entityState, ctx);
    }
  }
}

void FunctionPrototype::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
  auto emitCtx = EmitCtx::get(irCtx, mod);
  if (checker.has_value()) {
    auto cond = checker.value()->emit(emitCtx);
    if (cond->get_ir_type()->is_bool()) {
      if (!llvm::cast<llvm::ConstantInt>(cond->get_llvm_constant())->getValue().getBoolValue()) {
        checkResult = false;
        return;
      }
    } else {
      irCtx->Error("The condition for defining a function should be of " + irCtx->color("bool") + " type",
                   checker.value()->fileRange);
    }
  }
  if (isGeneric()) {
    for (auto* gen : generics) {
      gen->emit(EmitCtx::get(irCtx, mod));
    }
    genericFn = new ir::GenericFunction(name, generics, genericConstraint, this, mod, emitCtx->getVisibInfo(visibSpec));
  } else {
    if (phase == ir::EmitPhase::phase_1) {
      function = create_function(mod, irCtx);
    } else if (phase == ir::EmitPhase::phase_2) {
      emit_definition(mod, irCtx);
    }
  }
}

ir::Function* FunctionPrototype::create_function(ir::Mod* mod, ir::Ctx* irCtx) const {
  SHOW("Creating function " << name.value << " with generic count: " << generics.size())
  auto emitCtx = EmitCtx::get(irCtx, mod);
  emitCtx->name_check_in_module(name, isGeneric() ? "generic function" : "function",
                                isGeneric() ? Maybe<String>(genericFn->get_id()) : None);
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
    if (!returnType.has_value()) {
      irCtx->Error(
          "The " + irCtx->color("main") + " function is required to always give a value of type " +
              irCtx->color("i32") +
              " to indicate the resultant status of the program to the underlying operating system. Give a " +
              irCtx->color("0") +
              " value at the end of the main function to indicate success, if you don't care much about the status",
          fileRange);
    }
  } else if (fnName == "main") {
    irCtx->Error("Main function cannot be inside a named module", name.range);
  }
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->is_type_member()) {
      irCtx->Error("Function is not a member function of a core type and cannot "
                   "use member argument syntax",
                   arg->get_name().range);
    }
    if (arg->get_type()) {
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
        if (generatedTypes.at(0)->is_pointer() && generatedTypes.at(0)->as_pointer()->isMulti() &&
            generatedTypes.at(0)->as_pointer()->getOwner().isAnonymous()) {
          if (generatedTypes.at(0)->as_pointer()->isSubtypeVariable()) {
            irCtx->Error("Type of the first argument of the " + irCtx->color("main") +
                             " function, cannot be a pointer with variability. "
                             "It should be of " +
                             irCtx->color("multiptr:[cstring ?]") + " type",
                         arguments.at(0)->get_type()->fileRange);
          }
          mod->set_has_main_function();
          irCtx->hasMain = true;
        } else {
          irCtx->Error("Type of the first argument of the " + irCtx->color("main") + " function should be " +
                           irCtx->color("multiptr:[cstring ?]"),
                       arguments.at(0)->get_type()->fileRange);
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
    if (!arguments.empty()) {
      args.push_back(
          // NOLINTNEXTLINE(readability-magic-numbers)
          ir::Argument::Create(
              Identifier(arguments.at(0)->get_name().value + "'count", arguments.at(0)->get_name().range),
              ir::UnsignedType::get(32u, irCtx), 0u));
      args.push_back(ir::Argument::Create(
          Identifier(arguments.at(0)->get_name().value + "'data", arguments.at(0)->get_name().range),
          ir::PointerType::get(false, ir::CType::get_cstr(irCtx), true, ir::PointerOwner::OfAnonymous(), false, irCtx),
          1u));
    }
  } else {
    for (usize i = 0; i < generatedTypes.size(); i++) {
      args.push_back(arguments.at(i)->is_variable()
                         ? ir::Argument::CreateVariable(arguments.at(i)->get_name(),
                                                        arguments.at(i)->get_type()->emit(EmitCtx::get(irCtx, mod)), i)
                         : ir::Argument::Create(arguments.at(i)->get_name(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  auto* retTy = returnType.has_value() ? returnType.value()->emit(emitCtx) : ir::VoidType::get(irCtx->llctx);
  if (isMainFn) {
    if (!(retTy->is_integer() && (retTy->as_integer()->get_bitwidth() == 32u))) {
      irCtx->Error(irCtx->color("main") + " function expects to have a given type of " + irCtx->color("i32"),
                   fileRange);
    }
  }
  Maybe<ir::MetaInfo> irMetaInfo;
  if (metaInfo.has_value()) {
    irMetaInfo = metaInfo.value().toIR(emitCtx);
  }
  if (isGeneric()) {
    Vec<ir::GenericParameter*> genericTypes;
    for (auto* gen : generics) {
      genericTypes.push_back(gen->toIRGenericType());
    }
    SHOW("About to create generic function")
    auto* fun = ir::Function::Create(mod, Identifier(fnName, name.range), None, std::move(genericTypes),
                                     ir::ReturnType::get(retTy), args, isVariadic, fileRange,
                                     emitCtx->getVisibInfo(visibSpec), irCtx, None, irMetaInfo);
    SHOW("Created IR function")
    return fun;
  } else {
    SHOW("About to create function")
    auto* fun = ir::Function::Create(
        mod, Identifier(fnName, name.range),
        isMainFn
            ? Maybe<LinkNames>(LinkNames({LinkNameUnit(irCtx->clangTargetInfo->getTriple().isWasm() ? "_start" : "main",
                                                       LinkUnitType::function)},
                                         "C", nullptr))
            : None,
        {}, ir::ReturnType::get(retTy), args, isVariadic, fileRange, emitCtx->getVisibInfo(visibSpec), irCtx,
        definition.has_value()
            ? None
            : Maybe<llvm::GlobalValue::LinkageTypes>(llvm::GlobalValue::LinkageTypes::ExternalLinkage),
        irMetaInfo);
    SHOW("Created IR function")
    return fun;
  }
}

void FunctionPrototype::emit_definition(ir::Mod* mod, ir::Ctx* irCtx) {
  SHOW("Getting IR function from prototype for " << name.value)
  auto* fnEmit = function;
  SHOW("Set active function: " << fnEmit->get_full_name())
  auto* block = new ir::Block(fnEmit, nullptr);
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
      auto* cmdArgsVal = block->new_value(
          fnEmit->arg_name_at(0).value.substr(0, fnEmit->arg_name_at(0).value.find('\'')),
          ir::PointerType::get(false, ir::CType::get_cstr(irCtx), false, ir::PointerOwner::OfAnonymous(), true, irCtx),
          false, fnEmit->arg_name_at(0).range);
      SHOW("Storing argument pointer")
      irCtx->builder.CreateStore(
          fnEmit->get_llvm_function()->getArg(1u),
          irCtx->builder.CreateStructGEP(cmdArgsVal->get_ir_type()->get_llvm_type(), cmdArgsVal->get_llvm(), 0u));
      SHOW("Storing argument count")
      irCtx->builder.CreateStore(
          irCtx->builder.CreateIntCast(fnEmit->get_llvm_function()->getArg(0u), llvm::Type::getInt64Ty(irCtx->llctx),
                                       false),
          irCtx->builder.CreateStructGEP(cmdArgsVal->get_ir_type()->get_llvm_type(), cmdArgsVal->get_llvm(), 1u));
    }
  } else {
    SHOW("About to allocate necessary arguments")
    auto argIRTypes = fnEmit->get_ir_type()->as_function()->get_argument_types();
    SHOW("Iteration run for function is: " << fnEmit->get_name().value)
    for (usize i = 0; i < argIRTypes.size(); i++) {
      auto argType = argIRTypes[i]->get_type();
      if (!argType->is_trivially_copyable() || argIRTypes[i]->is_variable()) {
        SHOW("Argument name is " << argIRTypes[i]->get_name())
        SHOW("Argument type is " << argType->to_string())
        auto* argVal = block->new_value(argIRTypes[i]->get_name(), argType, argIRTypes[i]->is_variable(),
                                        arguments[i]->get_name().range);
        SHOW("Created local value for the argument")
        irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), argVal->getAlloca(), false);
      }
    }
  }
  SHOW("Emitting sentences")
  emit_sentences(definition.value().first, EmitCtx::get(irCtx, mod)->with_function(fnEmit));
  SHOW("Sentences emitted")
  ir::function_return_handler(irCtx, fnEmit, fileRange);
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
      ._("isVariadic", isVariadic)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
      ._("hasMetaInfo", metaInfo.has_value())
      ._("metaInfo", metaInfo.has_value() ? metaInfo.value().to_json() : JsonValue())
      ._("sentences", sntcs)
      ._("bodyRange", definition.has_value() ? definition.value().second : JsonValue());
}

} // namespace qat::ast
