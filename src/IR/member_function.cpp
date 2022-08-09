#include "./member_function.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/pointer.hpp"
#include "types/qat_type.hpp"
#include "types/void.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

MemberFunction::MemberFunction(bool _isVariation, CoreType *_parent,
                               String _name, QatType *returnType,
                               bool _isReturnTypeVariable, bool _is_async,
                               Vec<Argument> _args, bool is_variable_arguments,
                               bool                   _is_static,
                               const utils::FileRange _fileRange,
                               utils::VisibilityInfo  _visibility_info,
                               llvm::LLVMContext     &ctx)
    : Function(_parent->getParent(), _name, returnType, _isReturnTypeVariable,
               _is_async, std::move(_args), is_variable_arguments, _fileRange,
               _visibility_info, ctx),
      parent(_parent), isStatic(_is_static), isVariation(_isVariation) {}

MemberFunction *MemberFunction::Create(
    CoreType *parent, const bool is_variation, const String name,
    QatType *returnTy, const bool _isReturnTypeVariable, const bool _is_async,
    const Vec<Argument> args, const bool has_variadic_args,
    const utils::FileRange      fileRange,
    const utils::VisibilityInfo visibilityInfo, llvm::LLVMContext &ctx) {
  Vec<Argument> args_info;
  args_info.push_back(is_variation ? Argument::CreateVariable("self", parent, 0)
                                   : Argument::Create("self", parent, 0));
  for (auto arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(
      is_variation, parent, name, returnTy, _isReturnTypeVariable, _is_async,
      args_info, has_variadic_args, false, fileRange, visibilityInfo, ctx);
}

MemberFunction *MemberFunction::CreateStatic(
    CoreType *parent, const String name, QatType *returnTy,
    const bool isReturnTypeVariable, const bool is_async,
    const Vec<Argument> args, const bool has_variadic_args,
    const utils::FileRange fileRange, const utils::VisibilityInfo visib_info,
    llvm::LLVMContext &ctx //
) {
  return new MemberFunction(false, parent, name, returnTy, isReturnTypeVariable,
                            is_async, args, has_variadic_args, true, fileRange,
                            visib_info, ctx);
}

MemberFunction *
MemberFunction::CreateDestructor(CoreType              *parent,
                                 const utils::FileRange fileRange,
                                 llvm::LLVMContext     &ctx) {
  return new MemberFunction(
      parent->getParent(), parent, "end", VoidType::get(ctx), false, false,
      Vec<Argument>(
          {Argument::CreateVariable("self", PointerType::get(parent, ctx), 0)}),
      false, false, fileRange, utils::VisibilityInfo::pub(), ctx);
}

bool MemberFunction::isVariationFunction() const {
  return isStatic ? false : isVariation;
}

String MemberFunction::getFullName() const {
  return parent->getFullName() + ":" + name;
}

bool MemberFunction::isStaticFunction() const { return isStatic; }

bool MemberFunction::isMemberFunction() const { return true; }

void MemberFunction::emitCPP(cpp::File &file) const {}

nuo::Json MemberFunction::toJson() const {}

} // namespace qat::IR