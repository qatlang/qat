#include "./member_function.hpp"
#include "../IR/types/core_type.hpp"
#include "../show.hpp"
#include "types/self_type.hpp"
#include <vector>

namespace qat::ast {

MemberPrototype::MemberPrototype(bool _isStatic, bool _isVariationFn, Identifier _name, Vec<Argument*> _arguments,
                                 bool _isVariadic, QatType* _returnType, Maybe<VisibilitySpec> _visibSpec,
                                 FileRange _fileRange)
    : Node(std::move(_fileRange)), isVariationFn(_isVariationFn), name(std::move(_name)),
      arguments(std::move(_arguments)), isVariadic(_isVariadic), returnType(_returnType), visibSpec(_visibSpec),
      isStatic(_isStatic) {}

MemberPrototype::~MemberPrototype() {
  for (auto* arg : arguments) {
    delete arg;
  }
}

MemberPrototype* MemberPrototype::Normal(bool _isVariationFn, const Identifier& _name, const Vec<Argument*>& _arguments,
                                         bool _isVariadic, QatType* _returnType, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
  return new MemberPrototype(false, _isVariationFn, _name, _arguments, _isVariadic, _returnType, visibSpec, _fileRange);
}

MemberPrototype* MemberPrototype::Static(const Identifier& _name, const Vec<Argument*>& _arguments, bool _isVariadic,
                                         QatType* _returnType, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
  return new MemberPrototype(true, false, _name, _arguments, _isVariadic, _returnType, visibSpec, _fileRange);
}

void MemberPrototype::define(IR::Context* ctx) {
  if (!memberParent) {
    ctx->Error("No parent type found for this member function", fileRange);
  }
  if (memberParent->isDoneSkill()) {
    auto doneSkill = memberParent->asDoneSkill();
    if ((isStatic || isVariationFn) && doneSkill->hasVariationFn(name.value)) {
      ctx->Error("A variation function named " + ctx->highlightError(name.value) +
                     " already exists in the same implementation at " +
                     ctx->highlightError(doneSkill->getVariationFn(name.value)->getName().range.startToString()),
                 name.range);
    }
    if ((isStatic || !isVariationFn) && doneSkill->hasNormalMemberFn(name.value)) {
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
  auto* parentType = memberParent->isDoneSkill() ? memberParent->asDoneSkill()->getType() : memberParent->asExpanded();
  if (parentType->isExpanded()) {
    auto expTy = parentType->asExpanded();
    if ((isStatic || isVariationFn) && expTy->hasVariationFn(name.value)) {
      ctx->Error("A variation function named " + ctx->highlightError(name.value) + " exists in the parent type " +
                     ctx->highlightError(expTy->getFullName()) + " at " +
                     ctx->highlightError(expTy->getVariationFn(name.value)->getName().range.startToString()),
                 name.range);
    }
    if ((isStatic || !isVariationFn) && expTy->hasNormalMemberFn(name.value)) {
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
                (memberParent->isDoneSkill() && memberParent->asDoneSkill()->isNormalSkill()
                     ? (" in the skill " + ctx->highlightError(memberParent->asDoneSkill()->getSkill()->getFullName()) +
                        " at " +
                        ctx->highlightError(memberParent->asDoneSkill()->getSkill()->getName().range.startToString()))
                     : ""),
            name.range);
      }
    }
  }
  bool isSelfReturn = false;
  if (returnType->typeKind() == TypeKind::selfType) {
    auto* selfRet = (SelfType*)returnType;
    if (!selfRet->isJustType) {
      selfRet->isVarRef          = isVariationFn;
      selfRet->canBeSelfInstance = true;
      isSelfReturn               = true;
    }
  }
  auto*             retTy = returnType->emit(ctx);
  Vec<IR::QatType*> generatedTypes;
  // TODO - Check existing member functions
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->isTypeMember()) {
      if (!memberParent->getParentType()->isCoreType()) {
        ctx->Error(
            "The parent type of this function is not a core type and hence the member argument syntax cannot be used",
            arg->getName().range);
      }
      if (!isStatic) {
        auto coreType = memberParent->getParentType()->asCore();
        if (coreType->hasMember(arg->getName().value)) {
          if (memberParent->isDoneSkill()) {
            if (!coreType->getMember(arg->getName().value)->visibility.isAccessible(ctx->getAccessInfo())) {
              ctx->Error("The member field " + ctx->highlightError(arg->getName().value) + " of parent type " +
                             ctx->highlightError(coreType->toString()) + " is not accessible here",
                         arg->getName().range);
            }
          }
          if (isVariationFn) {
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
        ctx->Error("Function " + name.value + " is a static function of type " +
                       memberParent->getParentType()->toString() + ". So it cannot use the member argument syntax",
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
  if (isStatic) {
    memberFn = IR::MemberFunction::CreateStatic(memberParent, name, retTy, args, isVariadic, fileRange,
                                                ctx->getVisibInfo(visibSpec), ctx);
  } else {
    memberFn = IR::MemberFunction::Create(memberParent, isVariationFn, name, IR::ReturnType::get(retTy, isSelfReturn),
                                          args, isVariadic, fileRange, ctx->getVisibInfo(visibSpec), ctx);
  }
}

IR::Value* MemberPrototype::emit(IR::Context* ctx) { return memberFn; }

void MemberPrototype::setMemberParent(IR::MemberParent* _memberParent) const { memberParent = _memberParent; }

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
      ._("isVariation", isVariationFn)
      ._("isStatic", isStatic)
      ._("name", name)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic);
}

MemberDefinition::MemberDefinition(MemberPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void MemberDefinition::define(IR::Context* ctx) { prototype->define(ctx); }

IR::Value* MemberDefinition::emit(IR::Context* ctx) {
  auto* fnEmit = (IR::MemberFunction*)prototype->emit(ctx);
  auto* oldFn  = ctx->setActiveFunction(fnEmit);
  SHOW("Set active member function: " << fnEmit->getFullName())
  auto* block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto* coreRefTy  = argIRTypes.at(0)->getType()->asReference();
  auto* self       = block->newValue("''", coreRefTy, false, coreRefTy->getSubType()->asCore()->getName().range);
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
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
      auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(),
                                     argIRTypes.at(i)->isVariable(), prototype->arguments.at(i - 1)->getName().range);
      SHOW("Created local value for the argument")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
    }
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

void MemberDefinition::setMemberParent(IR::MemberParent* memberParent) const {
  prototype->setMemberParent(memberParent);
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