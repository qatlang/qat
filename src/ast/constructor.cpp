#include "./constructor.hpp"
#include "../show.hpp"
#include "./sentences/member_initialisation.hpp"
#include "node.hpp"
#include <vector>

namespace qat::ast {

ConstructorPrototype::~ConstructorPrototype() {
  for (auto* arg : arguments) {
    std::destroy_at(arg);
  }
}

void ConstructorPrototype::define(MethodState& state, IR::Context* ctx) {
  if (type == ConstructorType::normal) {
    Vec<Pair<Maybe<bool>, IR::QatType*>> generatedTypes;
    bool                                 isMixAndHasMemberArg = false;
    SHOW("Generating types")
    for (auto* arg : arguments) {
      if (arg->isTypeMember()) {
        if (isMixAndHasMemberArg) {
          ctx->Error("Cannot have multiple member arguments in a constructor for type " +
                         ctx->highlightError(state.parent->getParentType()->toString()) + ", since it is a mix type",
                     arg->getName().range);
        }
        SHOW("Arg is type member")
        if (!state.parent->getParentType()->isCoreType() && !state.parent->getParentType()->isMix()) {
          ctx->Error("The parent type is not a core or mix type, and hence member argument syntax cannot be used here",
                     arg->getName().range);
        }
        if (state.parent->getParentType()->isCoreType()) {
          auto* coreType = state.parent->getParentType()->asCore();
          if (coreType->hasMember(arg->getName().value)) {
            for (auto mem : presentMembers) {
              if (mem->name.value == arg->getName().value) {
                ctx->Error("The member " + ctx->highlightError(mem->name.value) + " is repeating here",
                           arg->getName().range);
              }
            }
            auto* mem = coreType->getMember(arg->getName().value);
            mem->addMention(arg->getName().range);
            presentMembers.push_back(coreType->getMember(arg->getName().value));
            SHOW("Getting core type member: " << arg->getName().value)
            generatedTypes.push_back({None, mem->type});
          } else {
            ctx->Error("No member field named " + arg->getName().value + " in the core type " + coreType->getFullName(),
                       arg->getName().range);
          }
        } else {
          auto mixTy = state.parent->getParentType()->asMix();
          auto check = mixTy->hasSubTypeWithName(arg->getName().value);
          if (!check.first) {
            ctx->Error("No variant named " + ctx->highlightError(arg->getName().value) + " is present in mix type " +
                           ctx->highlightError(mixTy->toString()),
                       arg->getName().range);
          }
          if (!check.second) {
            ctx->Error("The variant " + ctx->highlightError(arg->getName().value) + " of mix type " +
                           ctx->highlightError(mixTy->toString()) +
                           " does not have a type associated with it and hence cannot be used as a member argument",
                       fileRange);
          }
          isMixAndHasMemberArg = true;
        }
      } else {
        generatedTypes.push_back({None, arg->getType()->emit(ctx)});
      }
      SHOW("Generated type of the argument is " << generatedTypes.back().second->toString())
    }
    SHOW("Argument types generated")
    if (state.parent->isDoneSkill() && state.parent->asDoneSkill()->hasConstructorWithTypes(generatedTypes)) {
      ctx->Error("A constructor with the same signature exists in the same implementation " +
                     ctx->highlightError(state.parent->asDoneSkill()->toString()),
                 fileRange,
                 Pair<String, FileRange>{
                     "The existing constructor can be found here",
                     state.parent->asDoneSkill()->getConstructorWithTypes(generatedTypes)->getName().range});
    }
    if (state.parent->getParentType()->isExpanded() &&
        state.parent->getParentType()->asExpanded()->hasConstructorWithTypes(generatedTypes)) {
      ctx->Error(
          "A constructor with the same signature exists in the parent type " +
              ctx->highlightError(state.parent->getParentType()->toString()),
          fileRange,
          Pair<String, FileRange>{
              "The existing constructor can be found here",
              state.parent->getParentType()->asExpanded()->getConstructorWithTypes(generatedTypes)->getName().range});
    }
    if (state.parent->getParentType()->isCoreType()) {
      auto  coreType   = state.parent->getParentType()->asCore();
      auto& allMembers = coreType->getMembers();
      for (auto* mem : allMembers) {
        bool foundMem = false;
        for (auto presentMem : presentMembers) {
          if (mem == presentMem) {
            foundMem = true;
            break;
          }
        }
        if (!foundMem) {
          if (mem->type->hasDefaultValue() || mem->type->isDefaultConstructible()) {
            absentMembersWithDefault.push_back(mem);
          } else {
            absentMembersWithoutDefault.push_back(mem);
          }
        }
      }
    }
    Vec<IR::Argument> args;
    SHOW("Setting variability of arguments")
    for (usize i = 0; i < generatedTypes.size(); i++) {
      if (arguments.at(i)->isTypeMember()) {
        SHOW("Creating member argument")
        args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(), generatedTypes.at(i).second, i));
      } else {
        args.push_back(
            arguments.at(i)->isVariable()
                ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
                : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i).second, i));
      }
    }
    SHOW("About to create function")
    state.result = IR::MemberFunction::CreateConstructor(state.parent, nameRange, args, false, fileRange,
                                                         ctx->getVisibInfo(visibSpec), ctx);
    SHOW("Constructor created in the IR")
  } else if (type == ConstructorType::Default) {
    if (state.parent->isDoneSkill() && state.parent->getParentType()->isExpanded() &&
        state.parent->getParentType()->asExpanded()->hasDefaultConstructor()) {
      ctx->Error("The target type of this implementation already has a " + ctx->highlightError("default") +
                     " constructor, so it cannot be defined again",
                 fileRange);
    }
    state.result =
        IR::MemberFunction::DefaultConstructor(state.parent, nameRange, ctx->getVisibInfo(visibSpec), fileRange, ctx);
  } else if (type == ConstructorType::copy) {
    state.result = IR::MemberFunction::CopyConstructor(state.parent, nameRange, argName.value(), fileRange, ctx);
  } else if (type == ConstructorType::move) {
    state.result = IR::MemberFunction::MoveConstructor(state.parent, nameRange, argName.value(), fileRange, ctx);
  }
}

Json ConstructorPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("isMemberArg", arg->isTypeMember());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "constructorPrototype")
      ._("arguments", args)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

void ConstructorDefinition::define(MethodState& state, IR::Context* ctx) { prototype->define(state, ctx); }

IR::Value* ConstructorDefinition::emit(MethodState& state, IR::Context* ctx) {
  auto* fnEmit = state.result;
  SHOW("FNemit is " << fnEmit)
  auto* oldFn = ctx->setActiveFunction(fnEmit);
  SHOW("Set active contructor: " << fnEmit->getFullName())
  auto* block = new IR::Block(fnEmit, nullptr);
  block->setFileRange(fileRange);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes  = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto* parentRefTy = argIRTypes.at(0)->getType()->asReference();
  auto* self        = block->newValue("''", parentRefTy, false, parentRefTy->getSubType()->asCore()->getName().range);
  SHOW("Storing self instance")
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u), self->getLLVM());
  SHOW("Loading implicit ptr")
  self->loadImplicitPointer(ctx->builder);
  SHOW("Loaded self instance")
  if ((prototype->type == ConstructorType::copy) || (prototype->type == ConstructorType::move)) {
    auto* argVal = block->newValue(
        prototype->argName->value,
        IR::ReferenceType::get(prototype->type == ConstructorType::move, state.parent->getParentType(), ctx), false,
        prototype->argName->range);
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1), argVal->getAlloca(), false);
    argVal->loadImplicitPointer(ctx->builder);
  } else {
    for (usize i = 1; i < argIRTypes.size(); i++) {
      SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
      if (argIRTypes.at(i)->isMemberArgument()) {
        llvm::Value* memPtr;
        if (parentRefTy->getSubType()->isCoreType()) {
          memPtr = ctx->builder.CreateStructGEP(
              parentRefTy->getSubType()->getLLVMType(), self->getLLVM(),
              parentRefTy->getSubType()->asCore()->getIndexOf(argIRTypes.at(i)->getName()).value());
          fnEmit->addInitMember({parentRefTy->getSubType()->asCore()->getMemberIndex(argIRTypes.at(i)->getName()),
                                 prototype->arguments.at(i - 1)->getName().range});
        } else if (parentRefTy->getSubType()->isMix()) {
          memPtr = ctx->builder.CreatePointerCast(
              ctx->builder.CreateStructGEP(parentRefTy->getSubType()->getLLVMType(), self->getLLVM(), 1u),
              parentRefTy->getSubType()
                  ->asMix()
                  ->getSubTypeWithName(argIRTypes.at(i)->getName())
                  ->getLLVMType()
                  ->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace()));
          fnEmit->addInitMember({parentRefTy->getSubType()->asMix()->getIndexOfName(argIRTypes.at(i)->getName()),
                                 prototype->arguments.at(i)->getName().range});
        } else {
          ctx->Error("Cannot use member argument syntax for the parent type " +
                         ctx->highlightError(parentRefTy->getSubType()->toString()) +
                         " as it is not a mix or core type",
                     prototype->arguments.at(i - 1)->getName().range);
        }
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), memPtr);
      } else {
        SHOW("Argument is variable")
        auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(),
                                       prototype->arguments.at(i - 1)->isVariable(),
                                       prototype->arguments.at(i - 1)->getName().range);
        SHOW("Created local value for the argument")
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
      }
    }
  }
  for (auto sent : sentences) {
    if (sent->nodeType() == NodeType::MEMBER_INIT) {
      ((ast::MemberInit*)sent)->isAllowed = true;
    }
  }
  emitSentences(sentences, ctx);
  if (state.parent->getParentType()->isCoreType()) {
    SHOW("Setting default values for fields")
    auto coreTy = state.parent->getParentType()->asCore();
    for (usize i = 0; i < coreTy->getMemberCount(); i++) {
      auto mem = coreTy->getMemberAt(i);
      if (fnEmit->isMemberInitted(i)) {
        continue;
      }
      if (mem->defaultValue.has_value()) {
        if (mem->defaultValue.value()->hasTypeInferrance()) {
          mem->defaultValue.value()->asTypeInferrable()->setInferenceType(mem->type);
        }
        auto* memVal = mem->defaultValue.value()->emit(ctx);
        if (memVal->getType()->isSame(mem->type)) {
          if (memVal->isImplicitPointer()) {
            if (mem->type->isTriviallyCopyable() || mem->type->isTriviallyMovable()) {
              ctx->builder.CreateStore(ctx->builder.CreateLoad(mem->type->getLLVMType(), memVal->getLLVM()),
                                       ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
              if (!mem->type->isTriviallyCopyable()) {
                if (!memVal->isVariable()) {
                  ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                             mem->defaultValue.value()->fileRange);
                }
                ctx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->getLLVMType()), memVal->getLLVM());
              }
            } else {
              ctx->Error("This expression is of type " + ctx->highlightError(memVal->getType()->toString()) +
                             " which is not trivially copyable or trivially movable. Please use " +
                             ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                         mem->defaultValue.value()->fileRange);
            }
          } else {
            ctx->builder.CreateStore(memVal->getLLVM(),
                                     ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
          }
        } else if (memVal->isReference() && memVal->getType()->asReference()->getSubType()->isSame(mem->type)) {
          if (mem->type->isTriviallyCopyable() || mem->type->isTriviallyMovable()) {
            ctx->builder.CreateStore(ctx->builder.CreateLoad(mem->type->getLLVMType(), memVal->getLLVM()),
                                     ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
            if (!mem->type->isTriviallyCopyable()) {
              if (!memVal->getType()->asReference()->isSubtypeVariable()) {
                ctx->Error(
                    "This expression is a reference without variability and hence cannot be trivially moved from",
                    mem->defaultValue.value()->fileRange);
              }
              ctx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->getLLVMType()), memVal->getLLVM());
            }
          } else {
            ctx->Error("This expression is a reference to type " + ctx->highlightError(mem->type->toString()) +
                           " which is not trivially copyable or trivially movable. Please use " +
                           ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                       mem->defaultValue.value()->fileRange);
          }
        } else if (mem->type->isReference() && mem->type->asReference()->getSubType()->isSame(memVal->getType()) &&
                   memVal->isImplicitPointer() &&
                   (mem->type->asReference()->isSubtypeVariable() ? memVal->isVariable() : true)) {
          ctx->builder.CreateStore(memVal->getLLVM(),
                                   ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
        } else {
          ctx->Error("The expected type of the member field is " + ctx->highlightError(mem->type->toString()) +
                         " but the value provided is of type " + ctx->highlightError(memVal->getType()->toString()),
                     FileRange{mem->name.range, mem->defaultValue.value()->fileRange});
        }
        fnEmit->addInitMember({i, mem->defaultValue.value()->fileRange});
      } else if (mem->type->hasDefaultValue() || mem->type->isDefaultConstructible()) {
        if (mem->type->hasDefaultValue()) {
          ctx->Warning("Member field " + ctx->highlightWarning(mem->name.value) +
                           " is initialised using the default value of type " +
                           ctx->highlightWarning(mem->type->toString()) +
                           " at the end of this constructor. Try using " + ctx->highlightWarning("default") +
                           " instead to explicitly initialise the member field",
                       fileRange);
          ctx->builder.CreateStore(mem->type->getDefaultValue(ctx)->getLLVM(),
                                   ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
        } else {
          ctx->Warning("Member field " + ctx->highlightWarning(mem->name.value) +
                           " is default constructed at the end of this constructor. Try using " +
                           ctx->highlightWarning("default") + " instead to explicitly initialise the member field",
                       fileRange);
          mem->type->defaultConstructValue(
              ctx,
              new IR::Value(ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i),
                            IR::ReferenceType::get(true, mem->type, ctx), false, IR::Nature::temporary),
              fnEmit);
        }
        fnEmit->addInitMember({i, fileRange});
      }
    }
  }
  if (state.parent->getParentType()->isCoreType()) {
    Vec<Pair<String, FileRange>> missingMembers;
    auto                         cTy = state.parent->getParentType()->asCore();
    for (auto ind = 0; ind < cTy->getMemberCount(); ind++) {
      auto memCheck = fnEmit->isMemberInitted(ind);
      if (!memCheck.has_value()) {
        missingMembers.push_back({cTy->getMemberAt(ind)->name.value, fileRange});
      }
    }
    if (!missingMembers.empty()) {
      Vec<IR::QatError> errors;
      for (usize i = 0; i < missingMembers.size(); i++) {
        errors.push_back(IR::QatError("Member field " + ctx->highlightError(missingMembers[i].first) +
                                          " of parent type " + ctx->highlightError(cTy->getFullName()) +
                                          " is not initialised in this constructor",
                                      missingMembers[i].second));
      }
      ctx->Errors(errors);
    }
  } else if (state.parent->getParentType()->isMix()) {
    bool isMixInitialised = false;
    for (usize i = 0; i < state.parent->getParentType()->asMix()->getSubTypeCount(); i++) {
      auto memRes = fnEmit->isMemberInitted(i);
      if (memRes.has_value()) {
        isMixInitialised = true;
        break;
      }
    }
    if (!isMixInitialised) {
      ctx->Error("Mix type is not initialised in this constructor", fileRange);
    }
  }
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

Json ConstructorDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "constructorDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast