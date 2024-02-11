#include "./member_function.hpp"
#include "../IR/types/core_type.hpp"
#include "../IR/types/void.hpp"
#include "../show.hpp"
#include "types/self_type.hpp"
#include <vector>

namespace qat::ast {

MemberPrototype::MemberPrototype(AstMemberFnType _fnTy, Identifier _name, Maybe<PrerunExpression*> _condition,
                                 Vec<Argument*> _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                                 Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : fnTy(_fnTy), name(std::move(_name)), condition(_condition), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), visibSpec(_visibSpec), fileRange(_fileRange) {}

MemberPrototype::~MemberPrototype() {
  for (auto* arg : arguments) {
    std::destroy_at(arg);
  }
}

MemberPrototype* MemberPrototype::Normal(bool _isVariationFn, const Identifier& _name,
                                         Maybe<PrerunExpression*> _condition, const Vec<Argument*>& _arguments,
                                         bool _isVariadic, Maybe<QatType*> _returnType, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
  return std::construct_at(OwnNormal(MemberPrototype),
                           _isVariationFn ? AstMemberFnType::variation : AstMemberFnType::normal, _name, _condition,
                           _arguments, _isVariadic, _returnType, visibSpec, _fileRange);
}

MemberPrototype* MemberPrototype::Static(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                         const Vec<Argument*>& _arguments, bool _isVariadic,
                                         Maybe<QatType*> _returnType, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
  return std::construct_at(OwnNormal(MemberPrototype), AstMemberFnType::Static, _name, _condition, _arguments,
                           _isVariadic, _returnType, visibSpec, _fileRange);
}

MemberPrototype* MemberPrototype::Value(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                        const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                                        Maybe<VisibilitySpec> visibSpec, const FileRange& _fileRange) {
  return std::construct_at(OwnNormal(MemberPrototype), AstMemberFnType::valued, _name, _condition, _arguments,
                           _isVariadic, _returnType, visibSpec, _fileRange);
}

void MemberPrototype::define(MethodState& state, IR::Context* ctx) {
  if (condition.has_value()) {
    auto condRes = condition.value()->emit(ctx);
    if (!condRes->getType()->isBool()) {
      ctx->Error("The condition for defining the method should be of " + ctx->highlightError("bool") + " type",
                 condition.value()->fileRange);
    }
    state.defineCondition = llvm::cast<llvm::ConstantInt>(condRes->getLLVM())->getValue().getBoolValue();
    if (state.defineCondition.has_value() && !state.defineCondition.value()) {
      return;
    }
  }
  SHOW("Defining member proto " << name.value << " " << state.parent->getParentType()->toString())
  if (state.parent->isDoneSkill()) {
    auto doneSkill = state.parent->asDoneSkill();
    if ((fnTy != AstMemberFnType::normal) && doneSkill->hasVariationFn(name.value)) {
      ctx->Error("A variation function named " + ctx->highlightError(name.value) +
                     " already exists in the same implementation at " +
                     ctx->highlightError(doneSkill->getVariationFn(name.value)->getName().range.startToString()),
                 name.range);
    }
    if ((fnTy != AstMemberFnType::variation) && doneSkill->hasNormalMemberFn(name.value)) {
      ctx->Error("A member function named " + ctx->highlightError(name.value) +
                     " already exists in the same implementation at " +
                     ctx->highlightError(doneSkill->getNormalMemberFn(name.value)->getName().range.startToString()),
                 name.range);
    }
    if (doneSkill->hasStaticFunction(name.value)) {
      ctx->Error("A static function named " + ctx->highlightError(name.value) +
                     " already exists in the same implementation at " +
                     ctx->highlightError(doneSkill->getStaticFunction(name.value)->getName().range.startToString()),
                 name.range);
    }
  }
  auto* parentType = state.parent->isDoneSkill() ? state.parent->asDoneSkill()->getType() : state.parent->asExpanded();
  if (parentType->isExpanded()) {
    auto expTy = parentType->asExpanded();
    if ((fnTy != AstMemberFnType::normal) && expTy->hasVariationFn(name.value)) {
      ctx->Error("A variation function named " + ctx->highlightError(name.value) + " exists in the parent type " +
                     ctx->highlightError(expTy->getFullName()) + " at " +
                     ctx->highlightError(expTy->getVariationFn(name.value)->getName().range.startToString()),
                 name.range);
    }
    if ((fnTy != AstMemberFnType::variation) && expTy->hasNormalMemberFn(name.value)) {
      ctx->Error("A member function named " + ctx->highlightError(name.value) + " exists in the parent type " +
                     ctx->highlightError(expTy->getFullName()) + " at " +
                     ctx->highlightError(expTy->getNormalMemberFn(name.value)->getName().range.startToString()),
                 name.range);
    }
    if (expTy->hasStaticFunction(name.value)) {
      ctx->Error("A static function named " + ctx->highlightError(name.value) + " already exists in the parent type " +
                     ctx->highlightError(expTy->getFullName()) + " at " +
                     ctx->highlightError(expTy->getStaticFunction(name.value)->getName().range.startToString()),
                 name.range);
    }
    if (expTy->isCoreType()) {
      if (expTy->asCore()->hasMember(name.value) || expTy->asCore()->hasStatic(name.value)) {
        ctx->Error(
            String(expTy->asCore()->hasMember(name.value) ? "Member" : "Static") + " field named " +
                ctx->highlightError(name.value) + " exists in the parent type " +
                ctx->highlightError(expTy->toString()) + ". Try if you can change the name of this function" +
                (state.parent->isDoneSkill() && state.parent->asDoneSkill()->isNormalSkill()
                     ? (" in the skill " +
                        ctx->highlightError(state.parent->asDoneSkill()->getSkill()->get_full_name()) + " at " +
                        ctx->highlightError(state.parent->asDoneSkill()->getSkill()->get_name().range.startToString()))
                     : ""),
            name.range);
      }
    }
  }
  bool isSelfReturn = false;
  if (returnType.has_value() && returnType.value()->typeKind() == AstTypeKind::SELF_TYPE) {
    auto* selfRet = (SelfType*)returnType.value();
    if (!selfRet->isJustType) {
      selfRet->isVarRef          = fnTy == AstMemberFnType::variation;
      selfRet->canBeSelfInstance = true;
      isSelfReturn               = true;
    }
  }
  auto*             retTy = returnType.has_value() ? returnType.value()->emit(ctx) : IR::VoidType::get(ctx->llctx);
  Vec<IR::QatType*> generatedTypes;
  // TODO - Check existing member functions
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->isTypeMember()) {
      if (!state.parent->getParentType()->isCoreType()) {
        ctx->Error(
            "The parent type of this function is not a core type and hence the member argument syntax cannot be used",
            arg->getName().range);
      }
      if (fnTy != AstMemberFnType::Static && fnTy != AstMemberFnType::valued) {
        auto coreType = state.parent->getParentType()->asCore();
        if (coreType->hasMember(arg->getName().value)) {
          if (state.parent->isDoneSkill()) {
            if (!coreType->getMember(arg->getName().value)->visibility.isAccessible(ctx->getAccessInfo())) {
              ctx->Error("The member field " + ctx->highlightError(arg->getName().value) + " of parent type " +
                             ctx->highlightError(coreType->toString()) + " is not accessible here",
                         arg->getName().range);
            }
          }
          if (fnTy == AstMemberFnType::variation) {
            auto* memTy = coreType->getTypeOfMember(arg->getName().value);
            if (memTy->isReference()) {
              if (memTy->asReference()->isSubtypeVariable()) {
                memTy = memTy->asReference()->getSubType();
              } else {
                ctx->Error("Member " + ctx->highlightError(arg->getName().value) + " of core type " +
                               ctx->highlightError(coreType->getFullName()) +
                               " is not a variable reference and hence cannot "
                               "be reassigned",
                           arg->getName().range);
              }
            }
            generatedTypes.push_back(memTy);
          } else {
            ctx->Error("This member function is not marked as a variation. It "
                       "cannot use the member argument syntax",
                       fileRange);
          }
        } else {
          ctx->Error("No non-static member named " + arg->getName().value + " in the core type " +
                         coreType->getFullName(),
                     arg->getName().range);
        }
      } else {
        ctx->Error("Function " + name.value + " is not a normal or variation method of type " +
                       state.parent->getParentType()->toString() + ". So it cannot use the member argument syntax",
                   arg->getName().range);
      }
    } else {
      generatedTypes.push_back(arg->getType()->emit(ctx));
    }
  }
  SHOW("Argument types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    if (arguments.at(i)->isTypeMember()) {
      SHOW("Argument at " << i << " named " << arguments.at(i)->getName().value << " is a type member")
      args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(), generatedTypes.at(i), i));
    } else {
      SHOW("Argument at " << i << " named " << arguments.at(i)->getName().value << " is not a type member")
      args.push_back(
          arguments.at(i)->isVariable()
              ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
              : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  if (fnTy == AstMemberFnType::Static) {
    SHOW("MemberFn :: " << name.value << " Static Method")
    state.result = IR::MemberFunction::CreateStatic(state.parent, name, retTy, args, isVariadic, fileRange,
                                                    ctx->getVisibInfo(visibSpec), ctx);
  } else if (fnTy == AstMemberFnType::valued) {
    SHOW("MemberFn :: " << name.value << " Valued Method")
    if (!parentType->isTriviallyCopyable()) {
      ctx->Error("The parent type is not trivially copyable and hence cannot have value methods", fileRange);
    }
    state.result = IR::MemberFunction::CreateValued(state.parent, name, retTy, args, isVariadic, fileRange,
                                                    ctx->getVisibInfo(visibSpec), ctx);
  } else {
    SHOW("MemberFn :: " << name.value << " Method or Variation")
    state.result = IR::MemberFunction::Create(state.parent, fnTy == AstMemberFnType::variation, name,
                                              IR::ReturnType::get(retTy, isSelfReturn), args, isVariadic, fileRange,
                                              ctx->getVisibInfo(visibSpec), ctx);
  }
}

Json MemberPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("isMemberArg", arg->isTypeMember());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "memberPrototype")
      ._("functionType", member_fn_type_to_string(fnTy))
      ._("name", name)
      ._("hasReturnType", returnType.has_value())
      ._("returnType", returnType.has_value() ? returnType.value()->toJson() : JsonValue())
      ._("arguments", args)
      ._("isVariadic", isVariadic);
}

void MemberDefinition::define(MethodState& state, IR::Context* ctx) { prototype->define(state, ctx); }

IR::Value* MemberDefinition::emit(MethodState& state, IR::Context* ctx) {
  if (state.defineCondition.has_value() && !state.defineCondition.value()) {
    return nullptr;
  }
  auto* fnEmit = state.result;
  auto* oldFn  = ctx->setActiveFunction(fnEmit);
  SHOW("Set active member function: " << fnEmit->getFullName())
  auto* block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto               argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  IR::ReferenceType* coreRefTy  = nullptr;
  IR::LocalValue*    self       = nullptr;
  if (prototype->fnTy != AstMemberFnType::Static && prototype->fnTy != AstMemberFnType::valued) {
    coreRefTy = argIRTypes.at(0)->getType()->asReference();
    self      = block->newValue("''", coreRefTy, false, coreRefTy->getSubType()->asCore()->getName().range);
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u), self->getLLVM());
    self->loadImplicitPointer(ctx->builder);
  }
  SHOW("Arguments size is " << argIRTypes.size())
  for (usize i = 1; i < argIRTypes.size(); i++) {
    SHOW("Argument name in member function is " << argIRTypes.at(i)->getName())
    SHOW("Argument type in member function is " << argIRTypes.at(i)->getType()->toString())
    if (argIRTypes.at(i)->isMemberArgument()) {
      auto* memPtr = ctx->builder.CreateStructGEP(
          coreRefTy->getSubType()->getLLVMType(), self->getLLVM(),
          coreRefTy->getSubType()->asCore()->getIndexOf(argIRTypes.at(i)->getName()).value());
      auto* memTy = coreRefTy->getSubType()->asCore()->getTypeOfMember(argIRTypes.at(i)->getName());
      if (memTy->isReference()) {
        memPtr = ctx->builder.CreateLoad(memTy->asReference()->getLLVMType(), memPtr);
      }
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), memPtr, false);
    } else {
      if (!argIRTypes.at(i)->getType()->isTriviallyCopyable() || argIRTypes.at(i)->isVariable()) {
        auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(),
                                       argIRTypes.at(i)->isVariable(), prototype->arguments.at(i - 1)->getName().range);
        SHOW("Created local value for the argument")
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
      }
    }
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

Json MemberDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "memberDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast