#include "./member_function.hpp"
#include "../IR/types/core_type.hpp"
#include "../IR/types/void.hpp"
#include "../show.hpp"
#include "emit_ctx.hpp"
#include "types/self_type.hpp"
#include <vector>

namespace qat::ast {

MemberPrototype::MemberPrototype(AstMemberFnType _fnTy, Identifier _name, Maybe<PrerunExpression*> _condition,
                                 Vec<Argument*> _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                 Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : fnTy(_fnTy), name(std::move(_name)), condition(_condition), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), metaInfo(_metaInfo), visibSpec(_visibSpec),
      fileRange(_fileRange) {}

MemberPrototype::~MemberPrototype() {
  for (auto* arg : arguments) {
    std::destroy_at(arg);
  }
}

MemberPrototype* MemberPrototype::Normal(bool _isVariationFn, const Identifier& _name,
                                         Maybe<PrerunExpression*> _condition, const Vec<Argument*>& _arguments,
                                         bool _isVariadic, Maybe<Type*> _returnType, Maybe<MetaInfo> _metaInfo,
                                         Maybe<VisibilitySpec> visibSpec, const FileRange& _fileRange) {
  return std::construct_at(OwnNormal(MemberPrototype),
                           _isVariationFn ? AstMemberFnType::variation : AstMemberFnType::normal, _name, _condition,
                           _arguments, _isVariadic, _returnType, _metaInfo, visibSpec, _fileRange);
}

MemberPrototype* MemberPrototype::Static(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                         const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                         Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
  return std::construct_at(OwnNormal(MemberPrototype), AstMemberFnType::Static, _name, _condition, _arguments,
                           _isVariadic, _returnType, _metaInfo, visibSpec, _fileRange);
}

MemberPrototype* MemberPrototype::Value(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                        const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                        Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> visibSpec,
                                        const FileRange& _fileRange) {
  return std::construct_at(OwnNormal(MemberPrototype), AstMemberFnType::valued, _name, _condition, _arguments,
                           _isVariadic, _returnType, _metaInfo, visibSpec, _fileRange);
}

void MemberPrototype::define(MethodState& state, ir::Ctx* irCtx) {
  if (condition.has_value()) {
    auto condRes =
        condition.value()->emit(EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent));
    if (!condRes->get_ir_type()->is_bool()) {
      irCtx->Error("The condition for defining the method should be of " + irCtx->color("bool") + " type",
                   condition.value()->fileRange);
    }
    state.defineCondition = llvm::cast<llvm::ConstantInt>(condRes->get_llvm())->getValue().getBoolValue();
    if (state.defineCondition.has_value() && !state.defineCondition.value()) {
      return;
    }
  }
  SHOW("Defining member proto " << name.value << " " << state.parent->get_parent_type()->to_string())
  if (state.parent->is_done_skill()) {
    auto doneSkill = state.parent->as_done_skill();
    if ((fnTy != AstMemberFnType::normal) && doneSkill->has_variation_method(name.value)) {
      irCtx->Error("A variation function named " + irCtx->color(name.value) +
                       " already exists in the same implementation at " +
                       irCtx->color(doneSkill->get_variation_method(name.value)->get_name().range.start_to_string()),
                   name.range);
    }
    if ((fnTy != AstMemberFnType::variation) && doneSkill->has_normal_method(name.value)) {
      irCtx->Error("A member function named " + irCtx->color(name.value) +
                       " already exists in the same implementation at " +
                       irCtx->color(doneSkill->get_normal_method(name.value)->get_name().range.start_to_string()),
                   name.range);
    }
    if (doneSkill->has_static_method(name.value)) {
      irCtx->Error("A static function named " + irCtx->color(name.value) +
                       " already exists in the same implementation at " +
                       irCtx->color(doneSkill->get_static_method(name.value)->get_name().range.start_to_string()),
                   name.range);
    }
  }
  SHOW("Getting parent type")
  auto* parentType =
      state.parent->is_done_skill() ? state.parent->as_done_skill()->get_ir_type() : state.parent->as_expanded();
  if (parentType->is_expanded()) {
    auto expTy = parentType->as_expanded();
    if ((fnTy != AstMemberFnType::normal) && expTy->has_variation(name.value)) {
      irCtx->Error("A variation function named " + irCtx->color(name.value) + " exists in the parent type " +
                       irCtx->color(expTy->get_full_name()) + " at " +
                       irCtx->color(expTy->get_variation(name.value)->get_name().range.start_to_string()),
                   name.range);
    }
    if ((fnTy != AstMemberFnType::variation) && expTy->has_normal_method(name.value)) {
      irCtx->Error("A member function named " + irCtx->color(name.value) + " exists in the parent type " +
                       irCtx->color(expTy->get_full_name()) + " at " +
                       irCtx->color(expTy->get_normal_method(name.value)->get_name().range.start_to_string()),
                   name.range);
    }
    if (expTy->has_static_method(name.value)) {
      irCtx->Error("A static function named " + irCtx->color(name.value) + " already exists in the parent type " +
                       irCtx->color(expTy->get_full_name()) + " at " +
                       irCtx->color(expTy->get_static_method(name.value)->get_name().range.start_to_string()),
                   name.range);
    }
    if (expTy->is_struct()) {
      if (expTy->as_struct()->has_field_with_name(name.value) || expTy->as_struct()->has_static(name.value)) {
        irCtx->Error(
            String(expTy->as_struct()->has_field_with_name(name.value) ? "Member" : "Static") + " field named " +
                irCtx->color(name.value) + " exists in the parent type " + irCtx->color(expTy->to_string()) +
                ". Try if you can change the name of this function" +
                (state.parent->is_done_skill() && state.parent->as_done_skill()->is_normal_skill()
                     ? (" in the skill " + irCtx->color(state.parent->as_done_skill()->get_skill()->get_full_name()) +
                        " at " +
                        irCtx->color(state.parent->as_done_skill()->get_skill()->get_name().range.start_to_string()))
                     : ""),
            name.range);
      }
    }
  }
  SHOW("Checking self type")
  bool isSelfReturn = false;
  if (returnType.has_value() && returnType.value()->type_kind() == AstTypeKind::SELF_TYPE) {
    SHOW("Return is self")
    auto* selfRet = (SelfType*)returnType.value();
    if (!selfRet->isJustType) {
      selfRet->isVarRef          = fnTy == AstMemberFnType::variation;
      selfRet->canBeSelfInstance = true;
      isSelfReturn               = true;
    }
  }
  SHOW("Creating emit ctx")
  auto emitCtx = EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent);
  SHOW("Emitting return type")
  auto* retTy = returnType.has_value() ? returnType.value()->emit(emitCtx) : ir::VoidType::get(irCtx->llctx);
  SHOW("Generating types")
  Vec<ir::Type*> generatedTypes;
  // TODO - Check existing member functions
  for (auto* arg : arguments) {
    if (arg->is_type_member()) {
      if (!state.parent->get_parent_type()->is_struct()) {
        irCtx->Error(
            "The parent type of this function is not a core type and hence the member argument syntax cannot be used",
            arg->get_name().range);
      }
      if (fnTy != AstMemberFnType::Static && fnTy != AstMemberFnType::valued) {
        auto structType = state.parent->get_parent_type()->as_struct();
        if (structType->has_field_with_name(arg->get_name().value)) {
          if (state.parent->is_done_skill()) {
            if (!structType->get_field_with_name(arg->get_name().value)
                     ->visibility.is_accessible(emitCtx->get_access_info())) {
              irCtx->Error("The member field " + irCtx->color(arg->get_name().value) + " of parent type " +
                               irCtx->color(structType->to_string()) + " is not accessible here",
                           arg->get_name().range);
            }
          }
          if (fnTy == AstMemberFnType::variation) {
            auto* memTy = structType->get_type_of_field(arg->get_name().value);
            if (memTy->is_reference()) {
              if (memTy->as_reference()->isSubtypeVariable()) {
                memTy = memTy->as_reference()->get_subtype();
              } else {
                irCtx->Error("Member " + irCtx->color(arg->get_name().value) + " of core type " +
                                 irCtx->color(structType->get_full_name()) +
                                 " is not a variable reference and hence cannot "
                                 "be reassigned",
                             arg->get_name().range);
              }
            }
            generatedTypes.push_back(memTy);
          } else {
            irCtx->Error("This member function is not marked as a variation. It "
                         "cannot use the member argument syntax",
                         fileRange);
          }
        } else {
          irCtx->Error("No non-static member named " + arg->get_name().value + " in the core type " +
                           structType->get_full_name(),
                       arg->get_name().range);
        }
      } else {
        irCtx->Error("Function " + name.value + " is not a normal or variation method of type " +
                         state.parent->get_parent_type()->to_string() + ". So it cannot use the member argument syntax",
                     arg->get_name().range);
      }
    } else {
      generatedTypes.push_back(arg->get_type()->emit(emitCtx));
    }
  }
  SHOW("Argument types generated")
  Vec<ir::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    if (arguments.at(i)->is_type_member()) {
      SHOW("Argument at " << i << " named " << arguments.at(i)->get_name().value << " is a type member")
      args.push_back(ir::Argument::CreateMember(arguments.at(i)->get_name(), generatedTypes.at(i), i));
    } else {
      SHOW("Argument at " << i << " named " << arguments.at(i)->get_name().value << " is not a type member")
      args.push_back(
          arguments.at(i)->is_variable()
              ? ir::Argument::CreateVariable(arguments.at(i)->get_name(), arguments.at(i)->get_type()->emit(emitCtx), i)
              : ir::Argument::Create(arguments.at(i)->get_name(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  if (fnTy == AstMemberFnType::Static) {
    SHOW("MemberFn :: " << name.value << " Static Method")
    state.result = ir::Method::CreateStatic(state.parent, name, retTy, args, isVariadic, fileRange,
                                            emitCtx->get_visibility_info(visibSpec), irCtx);
  } else if (fnTy == AstMemberFnType::valued) {
    SHOW("MemberFn :: " << name.value << " Valued Method")
    if (!parentType->is_trivially_copyable()) {
      irCtx->Error("The parent type is not trivially copyable and hence cannot have value methods", fileRange);
    }
    state.result = ir::Method::CreateValued(state.parent, name, retTy, args, isVariadic, fileRange,
                                            emitCtx->get_visibility_info(visibSpec), irCtx);
  } else {
    SHOW("MemberFn :: " << name.value << " Method or Variation")
    state.result = ir::Method::Create(state.parent, fnTy == AstMemberFnType::variation, name,
                                      ir::ReturnType::get(retTy, isSelfReturn), args, isVariadic, fileRange,
                                      emitCtx->get_visibility_info(visibSpec), irCtx);
  }
}

Json MemberPrototype::to_json() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->get_name())
                     ._("type", arg->get_type() ? arg->get_type()->to_json() : Json())
                     ._("is_member_argument", arg->is_type_member());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "memberPrototype")
      ._("functionType", member_fn_type_to_string(fnTy))
      ._("name", name)
      ._("hasReturnType", returnType.has_value())
      ._("returnType", returnType.has_value() ? returnType.value()->to_json() : JsonValue())
      ._("arguments", args)
      ._("isVariadic", isVariadic);
}

void MethodDefinition::define(MethodState& state, ir::Ctx* irCtx) { prototype->define(state, irCtx); }

ir::Value* MethodDefinition::emit(MethodState& state, ir::Ctx* irCtx) {
  if (state.defineCondition.has_value() && !state.defineCondition.value()) {
    return nullptr;
  }
  auto* fnEmit = state.result;
  SHOW("Set active member function: " << fnEmit->get_full_name())
  auto* block = new ir::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->set_active(irCtx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto               argIRTypes = fnEmit->get_ir_type()->as_function()->get_argument_types();
  ir::ReferenceType* coreRefTy  = nullptr;
  ir::LocalValue*    self       = nullptr;
  if (prototype->fnTy != AstMemberFnType::Static && prototype->fnTy != AstMemberFnType::valued) {
    coreRefTy = argIRTypes.at(0)->get_type()->as_reference();
    self      = block->new_value("''", coreRefTy, false, coreRefTy->get_subtype()->as_struct()->get_name().range);
    irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(0u), self->get_llvm());
    self->load_ghost_reference(irCtx->builder);
  }
  SHOW("Arguments size is " << argIRTypes.size())
  for (usize i = 1; i < argIRTypes.size(); i++) {
    SHOW("Argument name in member function is " << argIRTypes.at(i)->get_name())
    SHOW("Argument type in member function is " << argIRTypes.at(i)->get_type()->to_string())
    if (argIRTypes.at(i)->is_member_argument()) {
      auto* memPtr = irCtx->builder.CreateStructGEP(
          coreRefTy->get_subtype()->get_llvm_type(), self->get_llvm(),
          coreRefTy->get_subtype()->as_struct()->get_index_of(argIRTypes.at(i)->get_name()).value());
      auto* memTy = coreRefTy->get_subtype()->as_struct()->get_type_of_field(argIRTypes.at(i)->get_name());
      if (memTy->is_reference()) {
        memPtr = irCtx->builder.CreateLoad(memTy->as_reference()->get_llvm_type(), memPtr);
      }
      irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), memPtr, false);
    } else {
      if (!argIRTypes.at(i)->get_type()->is_trivially_copyable() || argIRTypes.at(i)->is_variable()) {
        auto* argVal =
            block->new_value(argIRTypes.at(i)->get_name(), argIRTypes.at(i)->get_type(),
                             argIRTypes.at(i)->is_variable(), prototype->arguments.at(i - 1)->get_name().range);
        SHOW("Created local value for the argument")
        irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), argVal->get_alloca(), false);
      }
    }
  }
  emit_sentences(
      sentences,
      EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent)->with_function(fnEmit));
  ir::function_return_handler(irCtx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  return nullptr;
}

Json MethodDefinition::to_json() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->to_json());
  }
  return Json()
      ._("nodeType", "memberDefinition")
      ._("prototype", prototype->to_json())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast
