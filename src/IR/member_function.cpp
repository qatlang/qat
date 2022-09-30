#include "./member_function.hpp"
#include "../show.hpp"
#include "argument.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/pointer.hpp"
#include "types/qat_type.hpp"
#include "types/reference.hpp"
#include "types/void.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

MemberFunction::MemberFunction(MemberFnType _fnType, bool _isVariation,
                               CoreType *_parent, const String &_name,
                               QatType *returnType, bool _isReturnTypeVariable,
                               bool _is_async, Vec<Argument> _args,
                               bool is_variable_arguments, bool _is_static,
                               const utils::FileRange      &_fileRange,
                               const utils::VisibilityInfo &_visibility_info,
                               llvm::LLVMContext           &ctx)
    : Function(_parent->getParent(),
               _parent->getFullName() + (_is_static ? ":" : "'") + _name,
               returnType, _isReturnTypeVariable, _is_async, std::move(_args),
               is_variable_arguments, _fileRange, _visibility_info, ctx, true),
      parent(_parent), isStatic(_is_static), isVariation(_isVariation),
      fnType(_fnType) {
  switch (fnType) {
  case MemberFnType::defaultConstructor: {
    parent->defaultConstructor = this;
    break;
  }
  case MemberFnType::constructor: {
    parent->constructors.push_back(this);
    break;
  }
  case MemberFnType::normal: {
    selfName = _name;
    parent->memberFunctions.push_back(this);
    break;
  }
  case MemberFnType::staticFn: {
    selfName = _name;
    parent->staticFunctions.push_back(this);
    break;
  }
  case MemberFnType::fromConvertor: {
    parent->fromConvertors.push_back(this);
    break;
  }
  case MemberFnType::toConvertor: {
    parent->toConvertors.push_back(this);
    break;
  }
  case MemberFnType::copyConstructor: {
    parent->copyConstructor = this;
    break;
  }
  case MemberFnType::moveConstructor: {
    parent->moveConstructor = this;
    break;
  }
  case MemberFnType::copyAssignment: {
    parent->copyAssignment = this;
    break;
  }
  case MemberFnType::moveAssignment: {
    parent->moveAssignment = this;
    break;
  }
  case MemberFnType::destructor: {
    parent->destructor = this;
    break;
  }
  case MemberFnType::binaryOperator: {
    selfName = _name;
    parent->binaryOperators.push_back(this);
    break;
  }
  case MemberFnType::unaryOperator: {
    selfName = _name;
    parent->unaryOperators.push_back(this);
    break;
  }
  }
}

MemberFunction *MemberFunction::Create(
    CoreType *parent, bool is_variation, const String &name, QatType *returnTy,
    bool _isReturnTypeVariable, bool _is_async, const Vec<Argument> &args,
    bool has_variadic_args, const utils::FileRange &fileRange,
    const utils::VisibilityInfo &visibilityInfo, llvm::LLVMContext &ctx) {
  Vec<Argument> args_info;
  args_info.push_back(
      Argument::Create("''", PointerType::get(is_variation, parent, ctx), 0));
  for (const auto &arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(MemberFnType::normal, is_variation, parent, name,
                            returnTy, _isReturnTypeVariable, _is_async,
                            std::move(args_info), has_variadic_args, false,
                            fileRange, visibilityInfo, ctx);
}

MemberFunction *MemberFunction::DefaultConstructor(
    CoreType *parent, const utils::VisibilityInfo &visibInfo,
    utils::FileRange fileRange, llvm::LLVMContext &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0));
  return new MemberFunction(MemberFnType::defaultConstructor, true, parent,
                            "default", IR::VoidType::get(ctx), false, false,
                            std::move(argsInfo), false, false,
                            {"", {0u, 0u}, {0u, 0u}}, visibInfo, ctx);
}

MemberFunction *
MemberFunction::CopyConstructor(CoreType *parent, const String &otherName,
                                const utils::FileRange &fileRange,
                                llvm::LLVMContext      &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0));
  argsInfo.push_back(
      Argument::Create(otherName, ReferenceType::get(false, parent, ctx), 0));
  return new MemberFunction(MemberFnType::copyConstructor, true, parent, "copy",
                            IR::VoidType::get(ctx), false, false,
                            std::move(argsInfo), false, false, fileRange,
                            utils::VisibilityInfo::pub(), ctx);
}

MemberFunction *
MemberFunction::MoveConstructor(CoreType *parent, const String &otherName,
                                const utils::FileRange &fileRange,
                                llvm::LLVMContext      &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0u));
  argsInfo.push_back(
      Argument::Create(otherName, ReferenceType::get(true, parent, ctx), 0u));
  return new MemberFunction(MemberFnType::moveConstructor, true, parent, "move",
                            IR::VoidType::get(ctx), false, false,
                            std::move(argsInfo), false, false, fileRange,
                            utils::VisibilityInfo::pub(), ctx);
}

MemberFunction *
MemberFunction::CopyAssignment(CoreType *parent, const String &otherName,
                               const utils::FileRange &fileRange,
                               llvm::LLVMContext      &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0u));
  argsInfo.push_back(
      Argument::Create(otherName, ReferenceType::get(true, parent, ctx), 0u));
  return new MemberFunction(MemberFnType::copyAssignment, true, parent,
                            "copy=", IR::VoidType::get(ctx), false, false,
                            std::move(argsInfo), false, false, fileRange,
                            utils::VisibilityInfo::pub(), ctx);
}

MemberFunction *
MemberFunction::MoveAssignment(CoreType *parent, const String &otherName,
                               const utils::FileRange &fileRange,
                               llvm::LLVMContext      &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0u));
  argsInfo.push_back(
      Argument::Create(otherName, ReferenceType::get(true, parent, ctx), 0u));
  return new MemberFunction(MemberFnType::moveAssignment, true, parent,
                            "move=", IR::VoidType::get(ctx), false, false,
                            std::move(argsInfo), false, false, fileRange,
                            utils::VisibilityInfo::pub(), ctx);
}

MemberFunction *MemberFunction::CreateConstructor(
    CoreType *parent, const Vec<Argument> &args, bool hasVariadicArgs,
    const utils::FileRange &fileRange, const utils::VisibilityInfo &visibInfo,
    llvm::LLVMContext &ctx) {
  Vec<Argument> argsInfo;
  // FIXME - Change parent pointer type to reference type?
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0));
  for (const auto &arg : args) {
    argsInfo.push_back(arg);
  }
  return new MemberFunction(MemberFnType::constructor, true, parent,
                            "from'" +
                                std::to_string(parent->constructors.size()),
                            VoidType::get(ctx), false, false, argsInfo,
                            hasVariadicArgs, false, fileRange, visibInfo, ctx);
}

MemberFunction *MemberFunction::CreateFromConvertor(
    CoreType *parent, QatType *sourceType, const String &name,
    const utils::FileRange &fileRange, const utils::VisibilityInfo &visibInfo,
    llvm::LLVMContext &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(true, parent, ctx), 0));
  SHOW("Created parent pointer argument")
  argsInfo.push_back(Argument::Create(name, sourceType, 1));
  SHOW("Created candidate type")
  return new MemberFunction(MemberFnType::fromConvertor, true, parent,
                            "from'" + sourceType->toString(),
                            VoidType::get(ctx), false, false, argsInfo, false,
                            false, fileRange, visibInfo, ctx);
}

MemberFunction *MemberFunction::CreateToConvertor(
    CoreType *parent, QatType *destType, const utils::FileRange &fileRange,
    const utils::VisibilityInfo &visibInfo, llvm::LLVMContext &ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create("''", PointerType::get(false, parent, ctx), 0));
  return new MemberFunction(MemberFnType::toConvertor, false, parent,
                            "to'" + destType->toString(), destType, false,
                            false, argsInfo, false, false, fileRange, visibInfo,
                            ctx);
}

MemberFunction *MemberFunction::CreateStatic(
    CoreType *parent, const String &name, QatType *returnTy,
    bool isReturnTypeVariable, bool is_async, const Vec<Argument> &args,
    bool has_variadic_args, const utils::FileRange &fileRange,
    const utils::VisibilityInfo &visib_info,
    llvm::LLVMContext           &ctx //
) {
  return new MemberFunction(MemberFnType::staticFn, false, parent, name,
                            returnTy, isReturnTypeVariable, is_async, args,
                            has_variadic_args, true, fileRange, visib_info,
                            ctx);
}

MemberFunction *
MemberFunction::CreateDestructor(CoreType               *parent,
                                 const utils::FileRange &fileRange,
                                 llvm::LLVMContext      &ctx) {
  SHOW("Creating destructor")
  return new MemberFunction(
      MemberFnType::destructor, true, parent, "end", VoidType::get(ctx), false,
      false,
      Vec<Argument>(
          {Argument::Create("''", PointerType::get(true, parent, ctx), 0)}),
      false, false, fileRange, utils::VisibilityInfo::pub(), ctx);
}

MemberFunction *MemberFunction::CreateOperator(
    CoreType *parent, bool isBinary, bool isVariationFn, const String &opr,
    IR::QatType *returnType, const Vec<Argument> &args,
    const utils::FileRange &fileRange, const utils::VisibilityInfo &visibInfo,
    llvm::LLVMContext &ctx) {
  Vec<Argument> args_info;
  args_info.push_back(
      Argument::Create("''", PointerType::get(isVariationFn, parent, ctx), 0));
  for (const auto &arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(
      isBinary ? MemberFnType::binaryOperator : MemberFnType::unaryOperator,
      isVariationFn, parent,
      "operator'" + opr +
          (isBinary
               ? ("'" + std::to_string(parent->getOperatorVariantIndex(opr)))
               : ""),
      returnType, false, false, args_info, false, false, fileRange, visibInfo,
      ctx);
}

String MemberFunction::getName() const {
  switch (fnType) {
  case MemberFnType::normal:
  case MemberFnType::binaryOperator:
  case MemberFnType::unaryOperator:
  case MemberFnType::staticFn: {
    return selfName;
  }
  case MemberFnType::fromConvertor:
  case MemberFnType::toConvertor:
  case MemberFnType::constructor:
  case MemberFnType::copyConstructor:
  case MemberFnType::moveConstructor:
  case MemberFnType::destructor:
    return name;
  }
}

bool MemberFunction::isVariationFunction() const {
  return isStatic ? false : isVariation;
}

String MemberFunction::getFullName() const { return name; }

bool MemberFunction::isStaticFunction() const { return isStatic; }

bool MemberFunction::isMemberFunction() const { return true; }

CoreType *MemberFunction::getParentType() { return parent; }

MemberFnType MemberFunction::getMemberFnType() { return fnType; }

void MemberFunction::emitCPP(cpp::File &file) const {}

Json MemberFunction::toJson() const {
  // TODO - Implement remaining
  return Json()._("coreType", parent->getID());
}

} // namespace qat::IR