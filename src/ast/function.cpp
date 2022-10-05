#include "./function.hpp"
#include "../IR/control_flow.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "types/templated.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionPrototype::FunctionPrototype(String _name, Vec<Argument*> _arguments, bool _isVariadic, QatType* _returnType,
                                     bool _is_async, llvm::GlobalValue::LinkageTypes _linkageType, String _callingConv,
                                     utils::VisibilityKind _visibility, const utils::FileRange& _fileRange,
                                     Vec<ast::TemplatedType*> _templates)
    : Node(_fileRange), name(std::move(_name)), isAsync(_is_async), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), callingConv(std::move(_callingConv)), visibility(_visibility),
      linkageType(_linkageType), templates(std::move(_templates)) {}

FunctionPrototype::FunctionPrototype(const FunctionPrototype& ref)
    : Node(ref.fileRange), name(ref.name), isAsync(ref.isAsync), arguments(ref.arguments), isVariadic(ref.isVariadic),
      returnType(ref.returnType), callingConv(ref.callingConv), visibility(ref.visibility),
      linkageType(ref.linkageType), function(ref.function), templates(ref.templates) {}

bool FunctionPrototype::isTemplate() const { return !templates.empty(); }

Vec<TemplatedType*> FunctionPrototype::getTemplates() const { return templates; }

IR::Function* FunctionPrototype::createFunction(IR::Context* ctx) const {
  Vec<IR::QatType*> generatedTypes;
  auto*             mod      = ctx->getMod();
  bool              isMainFn = false;
  String            fnName   = name;
  SHOW("Creating function")
  if (templateFn) {
    fnName = variantName.value();
  }
  SHOW("Checking function exists or not")
  if (mod->hasFunction(fnName)) {
    ctx->Error("A function named " + fnName + " exists already in this scope", fileRange);
  }
  if ((fnName == "main") && (mod->getFullNameWithChild("main") == "main")) {
    SHOW("Is main function")
    linkageType = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage;
    isMainFn    = true;
  }
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->isTypeMember()) {
      ctx->Error("Function is not a member function of a core type and cannot "
                 "use member argument syntax",
                 arg->getFileRange());
    }
    auto* genType = arg->getType()->emit(ctx);
    generatedTypes.push_back(genType);
  }
  SHOW("Types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    args.push_back(
        arguments.at(i)->getType()->isVariable()
            ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
            : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  auto* fn = mod->createFunction(fnName, returnType->emit(ctx), returnType->isVariable(), isAsync, args, isVariadic,
                                 fileRange, ctx->getVisibInfo(visibility), linkageType, ctx->llctx);
  if (isMainFn) {
    if (ctx->hasMain) {
      ctx->Error(ctx->highlightError("main") + " function already exists. Please check the codebase", fileRange);
    } else {
      auto args = fn->getType()->asFunction()->getArgumentTypes();
      if (args.size() == 2) {
        if (args.at(0)->getType()->isUnsignedInteger() &&
            (args.at(0)->getType()->asUnsignedInteger()->getBitwidth() == MainFirstArgBitwidth)) {
          if (args.at(1)->getType()->isPointer() && ((!args.at(1)->getType()->asPointer()->isSubtypeVariable()) &&
                                                     (args.at(1)->getType()->asPointer()->getSubType()->isCString()))) {
            ctx->hasMain = true;
          } else {
            ctx->Error("The second argument of the " + ctx->highlightError("main") + " function should be " +
                           ctx->highlightError("#[cstring]"),
                       arguments.at(1)->getFileRange());
          }
        } else {
          ctx->Error("The first argument of the " + ctx->highlightError("main") + " function should be " +
                         ctx->highlightError("u32"),
                     arguments.at(0)->getFileRange());
        }
      } else if (args.size() == 0) {
        ctx->hasMain = true;
      } else {
        ctx->Error("Main function can either have two arguments, or none at "
                   "all. Please check logic and make necessary changes",
                   fileRange);
      }
      // else {
      //   ctx->Error(
      //       "The " + ctx->highlightError("main") +
      //           " function needs 2 arguments, the first argument should be "
      //           + ctx->highlightError("u32") +
      //           ", and the second argument should be " +
      //           ctx->highlightError("#[cstring]"),
      //       fileRange);
      // }
    }
  }
  return fn;
}

void FunctionPrototype::setVariantName(const String& value) const { variantName = value; }

void FunctionPrototype::unsetVariantName() const { variantName = None; }

void FunctionPrototype::define(IR::Context* ctx) {
  if (!isTemplate()) {
    function = createFunction(ctx);
  }
}

// NOLINTNEXTLINE(misc-unused-parameters)
IR::Value* FunctionPrototype::emit(IR::Context* ctx) {
  if (!isTemplate()) {
    return function;
  } else {
    function = createFunction(ctx);
    return function;
  }
}

Json FunctionPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("fileRange", arg->getFileRange());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "functionPrototype")
      ._("name", name)
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic)
      ._("callingConvention", callingConv);
}

FunctionDefinition::FunctionDefinition(FunctionPrototype* _prototype, Vec<Sentence*> _sentences,
                                       utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void FunctionDefinition::define(IR::Context* ctx) {
  if (!prototype->isTemplate()) {
    prototype->define(ctx);
  } else {
    auto* tempFn          = new IR::TemplateFunction(prototype->name, prototype->templates, this, ctx->getMod(),
                                                     ctx->getVisibInfo(prototype->visibility));
    prototype->templateFn = tempFn;
  }
}

bool FunctionDefinition::isTemplate() const { return prototype->isTemplate(); }

IR::Value* FunctionDefinition::emit(IR::Context* ctx) {
  if (prototype->isTemplate()) {
    (void)prototype->emit(ctx);
  }
  SHOW("Getting IR function from prototype")
  auto* fnEmit = prototype->function;
  ctx->fn      = fnEmit;
  SHOW("Set active function: " << fnEmit->getFullName())
  auto* block = new IR::Block((IR::Function*)fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  SHOW("Iteration run for function is: " << fnEmit->getName())
  for (usize i = 0; i < argIRTypes.size(); i++) {
    SHOW("Argument name is " << argIRTypes.at(i)->getName())
    SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
    auto* argVal =
        block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(), argIRTypes.at(i)->isVariable());
    SHOW("Created local value for the argument")
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, fileRange);
  SHOW("Sentences emitted")
  return fnEmit;
}

Json FunctionDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast