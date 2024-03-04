#include "convertor.hpp"
#include "../show.hpp"
#include "./expression.hpp"
#include "./sentences/member_initialisation.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include <algorithm>

namespace qat::ast {

void ConvertorPrototype::define(MethodState& state, ir::Ctx* irCtx) {
  SHOW("Generating candidate type")
  ir::Type* candidate = nullptr;
  if (isFrom && argName.has_value() && is_member_argumentument) {
    if (state.parent->get_parent_type()->is_struct()) {
      auto coreType = state.parent->get_parent_type()->as_struct();
      if (!coreType->has_field_with_name(argName->value)) {
        irCtx->Error("No member field named " + irCtx->color(argName->value) + " found in core type " +
                         irCtx->color(coreType->get_full_name()),
                     fileRange);
      }
      coreType->get_field_with_name(argName->value)->add_mention(argName->range);
      candidate = coreType->get_type_of_field(argName->value);
    } else if (state.parent->get_parent_type()->is_mix()) {
      auto mixTy  = state.parent->get_parent_type()->as_mix();
      auto mixRes = mixTy->has_variant_with_name(argName->value);
      if (!mixRes.first) {
        irCtx->Error("No variant named " + irCtx->color(argName->value) + " is present in mix type " +
                         irCtx->color(mixTy->to_string()),
                     argName->range);
      }
      if (!mixRes.second) {
        irCtx->Error("The variant named " + irCtx->color(argName->value) + " of mix type " +
                         irCtx->color(mixTy->to_string()) + " does not have a type associated with it",
                     argName->range);
      }
      candidate = mixTy->get_variant_with_name(argName->value);
    } else {
      irCtx->Error(
          "The parent type of this from convertor is not a mix or core type, and hence member argument syntax cannot be used here",
          argName->range);
    }
  } else {
    candidate = candidateType->emit(EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent));
  }
  SHOW("Candidate type generated")
  SHOW("About to create convertor")
  auto emitCtx = EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent);
  if (isFrom) {
    SHOW("Convertor is FROM for " << state.parent->get_parent_type()->to_string())
    state.result =
        ir::Method::CreateFromConvertor(state.parent, nameRange, candidate, argName.value(),
                                        definitionRange.value_or(fileRange), emitCtx->getVisibInfo(visibSpec), irCtx);
    SHOW("Convertor IR created")
  } else {
    SHOW("Convertor is TO")
    state.result =
        ir::Method::CreateToConvertor(state.parent, nameRange, candidate, definitionRange.value_or(fileRange),
                                      emitCtx->getVisibInfo(visibSpec), irCtx);
  }
}

Json ConvertorPrototype::to_json() const {
  return Json()
      ._("nodeType", "convertorPrototype")
      ._("isFrom", isFrom)
      ._("hasArgument", argName.has_value())
      ._("argumentName", argName.has_value() ? argName.value().operator JsonValue() : Json())
      ._("candidateType", candidateType->to_json())
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue());
}

void ConvertorDefinition::define(MethodState& state, ir::Ctx* irCtx) { prototype->define(state, irCtx); }

ir::Value* ConvertorDefinition::emit(MethodState& state, ir::Ctx* irCtx) {
  SHOW("Defining Convertor is " << (prototype->isFrom ? "FROM" : "TO") << " for type "
                                << state.parent->get_parent_type()->to_string())
  auto* fnEmit = state.result;
  SHOW("MemberFn name is " << fnEmit->get_full_name())
  SHOW("Set active convertor function: " << fnEmit->get_full_name())
  auto* block = new ir::Block((ir::Function*)fnEmit, nullptr);
  SHOW("Created entry block")
  block->set_active(irCtx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto* parentRefType = ir::ReferenceType::get(prototype->isFrom, state.parent->get_parent_type(), irCtx);
  auto* self          = block->new_value("''", parentRefType, false, state.parent->get_type_range());
  irCtx->builder.CreateStore(fnEmit->get_llvmFunction()->getArg(0), self->get_llvm());
  self->load_ghost_pointer(irCtx->builder);
  if (prototype->isFrom) {
    if (prototype->is_member_argumentument) {
      llvm::Value* memPtr = nullptr;
      if (parentRefType->get_subtype()->is_struct()) {
        memPtr = irCtx->builder.CreateStructGEP(
            parentRefType->get_subtype()->get_llvm_type(), self->get_llvm(),
            parentRefType->get_subtype()->as_struct()->get_index_of(prototype->argName->value).value());
        fnEmit->addInitMember(
            {parentRefType->get_subtype()->as_struct()->get_index_of(prototype->argName->value).value(),
             prototype->argName->range});
      } else {
        memPtr = irCtx->builder.CreatePointerCast(
            irCtx->builder.CreateStructGEP(parentRefType->get_subtype()->get_llvm_type(), self->get_llvm(), 1u),
            llvm::PointerType::get(parentRefType->get_subtype()
                                       ->as_mix()
                                       ->get_variant_with_name(prototype->argName->value)
                                       ->get_llvm_type(),
                                   irCtx->dataLayout.value().getProgramAddressSpace()));
        fnEmit->addInitMember({parentRefType->get_subtype()->as_mix()->get_index_of(prototype->argName->value),
                               prototype->argName->range});
      }
      SHOW("Storing member arg in from convertor")
      irCtx->builder.CreateStore(fnEmit->get_llvmFunction()->getArg(1), memPtr, false);
      if (parentRefType->get_subtype()->is_mix()) {
        auto* mixTy = parentRefType->get_subtype()->as_mix();
        irCtx->builder.CreateStore(
            llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, mixTy->get_tag_bitwidth()),
                                   mixTy->get_index_of(prototype->argName->value)),
            irCtx->builder.CreateStructGEP(mixTy->get_llvm_type(), self->get_llvm(), 0u));
      }
      SHOW("Member arg complete")
    } else {
      auto* argTy = fnEmit->get_ir_type()->as_function()->get_argument_type_at(1);
      auto* argVal =
          block->new_value(argTy->get_name(), argTy->get_type(), argTy->is_variable(), prototype->argName->range);
      irCtx->builder.CreateStore(fnEmit->get_llvmFunction()->getArg(1), argVal->get_llvm());
    }
  }
  for (auto sent : sentences) {
    if (sent->nodeType() == NodeType::MEMBER_INIT) {
      ((ast::MemberInit*)sent)->isAllowed = true;
    }
  }
  emit_sentences(
      sentences,
      EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent)->with_function(fnEmit));
  if (prototype->isFrom) {
    if (state.parent->get_parent_type()->is_struct()) {
      SHOW("Setting default values for fields")
      auto coreTy = state.parent->get_parent_type()->as_struct();
      for (usize i = 0; i < coreTy->get_field_count(); i++) {
        auto mem = coreTy->get_field_at(i);
        if (fnEmit->isMemberInitted(i)) {
          continue;
        }
        if (mem->defaultValue.has_value()) {
          auto* memVal = mem->defaultValue.value()->emit(
              EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent));
          if (memVal->get_ir_type()->is_same(mem->type)) {
            if (memVal->is_ghost_pointer()) {
              if (mem->type->is_trivially_copyable() || mem->type->is_trivially_movable()) {
                irCtx->builder.CreateStore(
                    irCtx->builder.CreateLoad(mem->type->get_llvm_type(), memVal->get_llvm()),
                    irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
                if (!mem->type->is_trivially_copyable()) {
                  if (!memVal->is_variable()) {
                    irCtx->Error("This expression does not have variability and hence cannot be trivially moved from",
                                 mem->defaultValue.value()->fileRange);
                  }
                  irCtx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->get_llvm_type()),
                                             memVal->get_llvm());
                }
              } else {
                irCtx->Error("This expression is of type " + irCtx->color(memVal->get_ir_type()->to_string()) +
                                 " which is not trivially copyable or trivially movable. Please use " +
                                 irCtx->color("'copy") + " or " + irCtx->color("'move") + " accordingly",
                             mem->defaultValue.value()->fileRange);
              }
            } else {
              irCtx->builder.CreateStore(memVal->get_llvm(),
                                         irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
            }
          } else if (memVal->is_reference() &&
                     memVal->get_ir_type()->as_reference()->get_subtype()->is_same(mem->type)) {
            if (mem->type->is_trivially_copyable() || mem->type->is_trivially_movable()) {
              irCtx->builder.CreateStore(irCtx->builder.CreateLoad(mem->type->get_llvm_type(), memVal->get_llvm()),
                                         irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
              if (!mem->type->is_trivially_copyable()) {
                if (!memVal->get_ir_type()->as_reference()->isSubtypeVariable()) {
                  irCtx->Error(
                      "This expression is a reference without variability and hence cannot be trivially moved from",
                      mem->defaultValue.value()->fileRange);
                }
                irCtx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->get_llvm_type()),
                                           memVal->get_llvm());
              }
            } else {
              irCtx->Error("This expression is a reference to type " + irCtx->color(mem->type->to_string()) +
                               " which is not trivially copyable or trivially movable. Please use " +
                               irCtx->color("'copy") + " or " + irCtx->color("'move") + " accordingly",
                           mem->defaultValue.value()->fileRange);
            }
          } else if (mem->type->is_reference() &&
                     mem->type->as_reference()->get_subtype()->is_same(memVal->get_ir_type()) &&
                     memVal->is_ghost_pointer() &&
                     (mem->type->as_reference()->isSubtypeVariable() ? memVal->is_variable() : true)) {
            irCtx->builder.CreateStore(memVal->get_llvm(),
                                       irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
          } else {
            irCtx->Error("The expected type of the member field is " + irCtx->color(mem->type->to_string()) +
                             " but the value provided is of type " + irCtx->color(memVal->get_ir_type()->to_string()),
                         FileRange{mem->name.range, mem->defaultValue.value()->fileRange});
          }
          fnEmit->addInitMember({i, mem->defaultValue.value()->fileRange});
        } else if (mem->type->has_prerun_default_value() || mem->type->is_default_constructible()) {
          if (mem->type->has_prerun_default_value()) {
            irCtx->Warning("Member field " + irCtx->highlightWarning(mem->name.value) +
                               " is initialised using the default value of type " +
                               irCtx->highlightWarning(mem->type->to_string()) +
                               " at the end of this convertor. Try using " + irCtx->highlightWarning("default") +
                               " instead to explicitly initialise the member field",
                           fileRange);
            irCtx->builder.CreateStore(mem->type->get_prerun_default_value(irCtx)->get_llvm(),
                                       irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
          } else {
            irCtx->Warning("Member field " + irCtx->highlightWarning(mem->name.value) +
                               " is default constructed at the end of this convertor. Try using " +
                               irCtx->highlightWarning("default") +
                               " instead to explicitly initialise the member field",
                           fileRange);
            mem->type->default_construct_value(
                irCtx,
                ir::Value::get(irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i),
                               ir::ReferenceType::get(true, mem->type, irCtx), false),
                fnEmit);
          }
          fnEmit->addInitMember({i, fileRange});
        }
      }
    }
    if (state.parent->get_parent_type()->is_struct()) {
      Vec<Pair<String, FileRange>> missingMembers;
      auto                         cTy = state.parent->get_parent_type()->as_struct();
      for (auto ind = 0; ind < cTy->get_field_count(); ind++) {
        auto memCheck = fnEmit->isMemberInitted(ind);
        if (!memCheck.has_value()) {
          missingMembers.push_back({cTy->get_field_at(ind)->name.value, fileRange});
        }
      }
      if (!missingMembers.empty()) {
        Vec<ir::QatError> errors;
        for (usize i = 0; i < missingMembers.size(); i++) {
          errors.push_back(ir::QatError("Member field " + irCtx->color(missingMembers[i].first) + " of parent type " +
                                            irCtx->color(cTy->get_full_name()) +
                                            " is not initialised in this convertor",
                                        missingMembers[i].second));
        }
        irCtx->Errors(errors);
      }
    } else if (state.parent->get_parent_type()->is_mix()) {
      bool isMixInitialised = false;
      for (usize i = 0; i < state.parent->get_parent_type()->as_mix()->get_variant_count(); i++) {
        auto memRes = fnEmit->isMemberInitted(i);
        if (memRes.has_value()) {
          isMixInitialised = true;
          break;
        }
      }
      if (!isMixInitialised) {
        irCtx->Error("Mix type is not initialised in this convertor", fileRange);
      }
    }
  }
  ir::function_return_handler(irCtx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  return nullptr;
}

Json ConvertorDefinition::to_json() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->to_json());
  }
  return Json()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->to_json())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast