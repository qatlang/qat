#include "./member_function.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/pointer.hpp"
#include "types/qat_type.hpp"
#include "types/void.hpp"

namespace qat::IR {

MemberFunction::MemberFunction(bool _isVariation, CoreType *_parent,
                               std::string _name, QatType *returnType,
                               bool _isReturnTypeVariable, bool _is_async,
                               std::vector<Argument> _args,
                               bool is_variable_arguments, bool _is_static,
                               const utils::FileRange _fileRange,
                               utils::VisibilityInfo _visibility_info)
    : parent(_parent), isVariation(_isVariation), isStatic(_is_static),
      Function(parent->getParent(), _name, returnType, _isReturnTypeVariable,
               _is_async, _args, is_variable_arguments, _fileRange,
               _visibility_info) {}

MemberFunction *MemberFunction::Create(
    CoreType *parent, const bool is_variation, const std::string name,
    QatType *returnTy, const bool _isReturnTypeVariable, const bool _is_async,
    const std::vector<Argument> args, const bool has_variadic_args,
    const utils::FileRange fileRange,
    const utils::VisibilityInfo visibilityInfo) {
  std::vector<Argument> args_info;
  args_info.push_back(is_variation ? Argument::CreateVariable("self", parent, 0)
                                   : Argument::Create("self", parent, 0));
  for (auto arg : args) {
    args_info.push_back(arg);
  }
  return new MemberFunction(
      is_variation, parent, name, returnTy, _isReturnTypeVariable, _is_async,
      args_info, has_variadic_args, false, fileRange, visibilityInfo);
}

MemberFunction *MemberFunction::CreateStatic(
    CoreType *parent, const std::string name, QatType *returnTy,
    const bool isReturnTypeVariable, const bool is_async,
    const std::vector<Argument> args, const bool has_variadic_args,
    const utils::FileRange fileRange,
    const utils::VisibilityInfo visib_info //
) {
  return new MemberFunction(false, parent, name, returnTy, isReturnTypeVariable,
                            is_async, args, has_variadic_args, true, fileRange,
                            visib_info);
}

MemberFunction *
MemberFunction::CreateDestructor(CoreType *parent,
                                 const utils::FileRange fileRange) {
  return new MemberFunction(
      parent->getParent(), parent, "end", new VoidType(), false, false,
      std::vector<Argument>(
          {Argument::CreateVariable("self", new PointerType(parent), 0)}),
      false, false, fileRange, utils::VisibilityInfo::pub());
}

bool MemberFunction::isVariationFunction() const {
  return isStatic ? false : isVariation;
}

std::string MemberFunction::getFullName() const {
  return parent->getFullName() + ":" + name;
}

bool MemberFunction::isStaticFunction() const { return isStatic; }

bool MemberFunction::isMemberFunction() const { return true; }

} // namespace qat::IR