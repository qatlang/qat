#include "./member_prototype.hpp"
#include "../show.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

MemberPrototype::MemberPrototype(bool _isStatic, bool _isVariationFn,
                                 const String   &_name,
                                 Vec<Argument *> _arguments, bool _isVariadic,
                                 QatType *_returnType, bool _is_async,
                                 utils::VisibilityKind   kind,
                                 const utils::FileRange &_fileRange)
    : Node(_fileRange), isVariationFn(_isVariationFn), name(_name),
      isAsync(_is_async), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), kind(kind),
      isStatic(_isStatic) {}

MemberPrototype *MemberPrototype::Normal(
    bool _isVariationFn, const String &_name, const Vec<Argument *> &_arguments,
    bool _isVariadic, QatType *_returnType, bool _is_async,
    utils::VisibilityKind kind, const utils::FileRange &_fileRange) {
  return new MemberPrototype(false, _isVariationFn, _name, _arguments,
                             _isVariadic, _returnType, _is_async, kind,
                             _fileRange);
}

MemberPrototype *MemberPrototype::Static(const String          &_name,
                                         const Vec<Argument *> &_arguments,
                                         bool _isVariadic, QatType *_returnType,
                                         bool                    _is_async,
                                         utils::VisibilityKind   kind,
                                         const utils::FileRange &_fileRange) {
  return new MemberPrototype(true, false, _name, _arguments, _isVariadic,
                             _returnType, _is_async, kind, _fileRange);
}

IR::Value *MemberPrototype::emit(IR::Context *ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  IR::MemberFunction *function;
  Vec<IR::QatType *>  generatedTypes;
  // TODO - Check existing member functions
  SHOW("Generating types")
  for (auto *arg : arguments) {
    if (arg->isTypeMember()) {
      if (!isStatic) {
        if (coreType->hasMember(arg->getName())) {
          if (isVariationFn) {
            generatedTypes.push_back(coreType->getTypeOfMember(arg->getName()));
          } else {
            ctx->Error("This member function is not marked as a variation. It "
                       "cannot use the member argument syntax",
                       fileRange);
          }
        } else {
          ctx->Error("No non-static member named " + arg->getName() +
                         " in the core type " + coreType->getFullName(),
                     arg->getFileRange());
        }
      } else {
        ctx->Error("Function " + name +
                       " is a static member function of the core type: " +
                       coreType->getFullName() +
                       ". So it "
                       "cannot use the member argument syntax",
                   arg->getFileRange());
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
      args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(),
                                                generatedTypes.at(i), i));
    } else {
      args.push_back(arguments.at(i)->getType()->isVariable()
                         ? IR::Argument::CreateVariable(
                               arguments.at(i)->getName(),
                               arguments.at(i)->getType()->emit(ctx), i)
                         : IR::Argument::Create(arguments.at(i)->getName(),
                                                generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  if (isStatic) {
    function = IR::MemberFunction::CreateStatic(
        coreType, name, returnType->emit(ctx), returnType->isVariable(),
        isAsync, args, isVariadic, fileRange, ctx->getVisibInfo(kind),
        ctx->llctx);
  } else {
    function = IR::MemberFunction::Create(
        coreType, isVariationFn, name, returnType->emit(ctx),
        returnType->isVariable(), isAsync, args, isVariadic, fileRange,
        ctx->getVisibInfo(kind), ctx->llctx);
  }
  SHOW("Function created!!")
  // TODO - Set calling convention
  return function;
}

void MemberPrototype::setCoreType(IR::CoreType *_coreType) {
  coreType = _coreType;
}

nuo::Json MemberPrototype::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto *arg : arguments) {
    auto aJson =
        nuo::Json()
            ._("name", arg->getName())
            ._("type", arg->getType() ? arg->getType()->toJson() : nuo::Json())
            ._("isMemberArg", arg->isTypeMember())
            ._("fileRange", arg->getFileRange());
    args.push_back(aJson);
  }
  return nuo::Json()
      ._("nodeType", "memberPrototype")
      ._("isVariation", isVariationFn)
      ._("isStatic", isStatic)
      ._("name", name)
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic);
}

} // namespace qat::ast