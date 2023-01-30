#include "./member_function.hpp"
#include "../memory_tracker.hpp"
#include "../show.hpp"
#include "./qat_module.hpp"
#include "argument.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/pointer.hpp"
#include "types/qat_type.hpp"
#include "types/reference.hpp"
#include "types/void.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

MemberFunction::MemberFunction(MemberFnType _fnType, bool _isVariation, ExpandedType* _parent, const Identifier& _name,
                               QatType* returnType, bool _isReturnTypeVariable, bool _is_async, Vec<Argument> _args,
                               bool is_variable_arguments, bool _is_static, const FileRange& _fileRange,
                               const utils::VisibilityInfo& _visibility_info, llvm::LLVMContext& ctx)
    : Function(_parent->getParent(),
               Identifier(_parent->getFullName() + (_is_static ? ":" : "'") + _name.value, _name.range),
               {/* Generics */}, returnType, _isReturnTypeVariable, _is_async, std::move(_args), is_variable_arguments,
               _fileRange, _visibility_info, ctx, true),
      parent(_parent), isStatic(_is_static), isVariation(_isVariation), fnType(_fnType), selfName(_name) {
  SHOW("Member function name :: " << name.value << " ; " << this)
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

String memberFnTypeToString(MemberFnType type) {
  switch (type) {
    case MemberFnType::normal:
      return "normalMemberFn";
    case MemberFnType::staticFn:
      return "staticFunction";
    case MemberFnType::fromConvertor:
      return "fromConvertor";
    case MemberFnType::toConvertor:
      return "toConvertor";
    case MemberFnType::constructor:
      return "constructor";
    case MemberFnType::copyConstructor:
      return "copyConstructor";
    case MemberFnType::moveConstructor:
      return "moveConstructor";
    case MemberFnType::copyAssignment:
      return "copyAssignment";
    case MemberFnType::moveAssignment:
      return "moveAssignment";
    case MemberFnType::destructor:
      return "destructor";
    case MemberFnType::binaryOperator:
      return "binaryOperator";
    case MemberFnType::unaryOperator:
      return "unaryOperator";
    case MemberFnType::defaultConstructor:
      return "defaultConstructor";
  }
}

MemberFunction::~MemberFunction() = default;

MemberFunction* MemberFunction::Create(ExpandedType* parent, bool is_variation, const Identifier& name,
                                       QatType* returnTy, bool _isReturnTypeVariable, bool _is_async,
                                       const Vec<Argument>& args, bool has_variadic_args, const FileRange& fileRange,
                                       const utils::VisibilityInfo& visibilityInfo, llvm::LLVMContext& ctx) {
  Vec<Argument> args_info;
  args_info.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(is_variation, parent, ctx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(MemberFnType::normal, is_variation, parent, name, returnTy, _isReturnTypeVariable,
                            _is_async, std::move(args_info), has_variadic_args, false, fileRange, visibilityInfo, ctx);
}

MemberFunction* MemberFunction::DefaultConstructor(ExpandedType* parent, FileRange nameRange,
                                                   const utils::VisibilityInfo& visibInfo, FileRange fileRange,
                                                   llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(true, parent, ctx), 0));
  return new MemberFunction(MemberFnType::defaultConstructor, true, parent, Identifier("default", nameRange),
                            IR::VoidType::get(ctx), false, false, std::move(argsInfo), false, false,
                            {"", {0u, 0u}, {0u, 0u}}, visibInfo, ctx);
}

MemberFunction* MemberFunction::CopyConstructor(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                                const FileRange& fileRange, llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create({"''", parent->getName().range}, ReferenceType::get(true, parent, ctx), 0));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(false, parent, ctx), 0));
  return new MemberFunction(MemberFnType::copyConstructor, true, parent, {"copy", std::move(nameRange)},
                            IR::VoidType::get(ctx), false, false, std::move(argsInfo), false, false, fileRange,
                            utils::VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::MoveConstructor(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                                const FileRange& fileRange, llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(Argument::Create({"''", parent->getName().range}, ReferenceType::get(true, parent, ctx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent, ctx), 0u));
  return new MemberFunction(MemberFnType::moveConstructor, true, parent, {"move", std::move(nameRange)},
                            IR::VoidType::get(ctx), false, false, std::move(argsInfo), false, false, fileRange,
                            utils::VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::CopyAssignment(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                               const FileRange& fileRange, llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(true, parent, ctx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent, ctx), 0u));
  return new MemberFunction(MemberFnType::copyAssignment, true, parent, {"copy=", nameRange}, IR::VoidType::get(ctx),
                            false, false, std::move(argsInfo), false, false, fileRange, utils::VisibilityInfo::pub(),
                            ctx);
}

MemberFunction* MemberFunction::MoveAssignment(ExpandedType* parent, FileRange nameRange, const Identifier& otherName,
                                               const FileRange& fileRange, llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(true, parent, ctx), 0u));
  argsInfo.push_back(Argument::Create(otherName, ReferenceType::get(true, parent, ctx), 0u));
  return new MemberFunction(MemberFnType::moveAssignment, true, parent, {"move=", nameRange}, IR::VoidType::get(ctx),
                            false, false, std::move(argsInfo), false, false, fileRange, utils::VisibilityInfo::pub(),
                            ctx);
}

MemberFunction* MemberFunction::CreateConstructor(ExpandedType* parent, FileRange nameRange, const Vec<Argument>& args,
                                                  bool hasVariadicArgs, const FileRange& fileRange,
                                                  const utils::VisibilityInfo& visibInfo, llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  // FIXME - Change parent pointer type to reference type?
  argsInfo.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(true, parent, ctx), 0));
  for (const auto& arg : args) {
    argsInfo.push_back(arg);
  }
  return new MemberFunction(MemberFnType::constructor, true, parent,
                            Identifier("from'" + std::to_string(parent->constructors.size()), std::move(nameRange)),
                            VoidType::get(ctx), false, false, argsInfo, hasVariadicArgs, false, fileRange, visibInfo,
                            ctx);
}

MemberFunction* MemberFunction::CreateFromConvertor(ExpandedType* parent, FileRange nameRange, QatType* sourceType,
                                                    const Identifier& name, const FileRange& fileRange,
                                                    const utils::VisibilityInfo& visibInfo, llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(true, parent, ctx), 0));
  SHOW("Created parent pointer argument")
  argsInfo.push_back(Argument::Create(name, sourceType, 1));
  SHOW("Created candidate type")
  return new MemberFunction(MemberFnType::fromConvertor, true, parent,
                            Identifier("from'" + sourceType->toString(), nameRange), VoidType::get(ctx), false, false,
                            argsInfo, false, false, fileRange, visibInfo, ctx);
}

MemberFunction* MemberFunction::CreateToConvertor(ExpandedType* parent, FileRange nameRange, QatType* destType,
                                                  const FileRange& fileRange, const utils::VisibilityInfo& visibInfo,
                                                  llvm::LLVMContext& ctx) {
  Vec<Argument> argsInfo;
  argsInfo.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(false, parent, ctx), 0));
  return new MemberFunction(MemberFnType::toConvertor, false, parent,
                            Identifier("to'" + destType->toString(), nameRange), destType, false, false, argsInfo,
                            false, false, fileRange, visibInfo, ctx);
}

MemberFunction* MemberFunction::CreateStatic(ExpandedType* parent, const Identifier& name, QatType* returnTy,
                                             bool isReturnTypeVariable, bool is_async, const Vec<Argument>& args,
                                             bool has_variadic_args, const FileRange& fileRange,
                                             const utils::VisibilityInfo& visib_info,
                                             llvm::LLVMContext&           ctx //
) {
  return new MemberFunction(MemberFnType::staticFn, false, parent, name, returnTy, isReturnTypeVariable, is_async, args,
                            has_variadic_args, true, fileRange, visib_info, ctx);
}

MemberFunction* MemberFunction::CreateDestructor(ExpandedType* parent, FileRange nameRange, const FileRange& fileRange,
                                                 llvm::LLVMContext& ctx) {
  SHOW("Creating destructor")
  return new MemberFunction(MemberFnType::destructor, true, parent, Identifier("end", nameRange), VoidType::get(ctx),
                            false, false,
                            Vec<Argument>({Argument::Create(Identifier("''", parent->getName().range),
                                                            ReferenceType::get(true, parent, ctx), 0)}),
                            false, false, fileRange, utils::VisibilityInfo::pub(), ctx);
}

MemberFunction* MemberFunction::CreateOperator(ExpandedType* parent, FileRange nameRange, bool isBinary,
                                               bool isVariationFn, const String& opr, IR::QatType* returnType,
                                               const Vec<Argument>& args, const FileRange& fileRange,
                                               const utils::VisibilityInfo& visibInfo, llvm::LLVMContext& ctx) {
  Vec<Argument> args_info;
  args_info.push_back(
      Argument::Create(Identifier("''", parent->getName().range), ReferenceType::get(isVariationFn, parent, ctx), 0));
  for (const auto& arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(
      isBinary ? MemberFnType::binaryOperator : MemberFnType::unaryOperator, isVariationFn, parent,
      Identifier("operator'" + opr + (isBinary ? ("'" + std::to_string(parent->getOperatorVariantIndex(opr))) : ""),
                 nameRange),
      returnType, false, false, args_info, false, false, fileRange, visibInfo, ctx);
}

Identifier MemberFunction::getName() const {
  switch (fnType) {
    case MemberFnType::normal:
    case MemberFnType::binaryOperator:
    case MemberFnType::unaryOperator:
    case MemberFnType::staticFn: {
      return selfName;
    }
    default:
      return name;
  }
}

bool MemberFunction::isVariationFunction() const { return isStatic ? false : isVariation; }

String MemberFunction::getFullName() const { return name.value; }

bool MemberFunction::isStaticFunction() const { return isStatic; }

bool MemberFunction::isMemberFunction() const { return true; }

ExpandedType* MemberFunction::getParentType() { return parent; }

MemberFnType MemberFunction::getMemberFnType() { return fnType; }

void MemberFunction::updateOverview() {
  Vec<JsonValue> localsJson;
  for (auto* block : blocks) {
    block->outputLocalOverview(localsJson);
  }
  ovRange = selfName.range;
  ovInfo._("fullName", getFullName())
      ._("selfName", selfName)
      ._("parentTypeID", parent->getID())
      ._("moduleID", parent->getParent()->getID())
      ._("isStatic", isStatic)
      ._("isVariation", isVariation)
      ._("memberFunctionType", memberFnTypeToString(fnType))
      ._("visibility", visibility_info)
      ._("isAsync", is_async)
      ._("isVariadic", hasVariadicArguments)
      ._("locals", localsJson);
}

Json MemberFunction::toJson() const {
  // TODO - Implement remaining
  return Json()._("coreType", parent->getID());
}

} // namespace qat::IR