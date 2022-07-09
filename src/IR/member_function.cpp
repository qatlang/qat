#include "./member_function.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/pointer.hpp"
#include "types/qat_type.hpp"
#include "types/void.hpp"

namespace qat {
namespace IR {

MemberFunction::MemberFunction(llvm::Module *module, CoreType *_parent,
                               std::string _name, QatType *returnType,
                               bool _is_async, std::vector<Argument> _args,
                               bool is_variable_arguments, bool _is_static,
                               bool _returns_reference,
                               const utils::FilePlacement _placement,
                               utils::VisibilityInfo _visibility_info)
    : parent(_parent),
      Function(module, parent->getName(), _name, returnType, _is_async, _args,
               is_variable_arguments, _returns_reference, _placement,
               _visibility_info) {}

MemberFunction *MemberFunction::Create(
    llvm::Module *mod, CoreType *parent, const bool is_variation,
    const std::string name, QatType *returnTy, const bool _is_async,
    const std::vector<Argument> args, const bool has_variadic_args,
    const bool returns_reference, const utils::FilePlacement placement,
    const utils::VisibilityInfo visib_info) {
  std::vector<Argument> args_info;
  args_info.push_back(is_variation ? Argument::CreateVariable("self", parent, 0)
                                   : Argument::Create("self", parent, 0));
  return new MemberFunction(mod, parent, name, returnTy, _is_async, args_info,
                            has_variadic_args, false, returns_reference,
                            placement, visib_info);
}

MemberFunction *MemberFunction::CreateStatic(
    llvm::Module *mod, CoreType *parent, const std::string name,
    QatType *returnTy, const bool is_async, const std::vector<Argument> args,
    const bool has_variadic_args, const bool returns_reference,
    const utils::FilePlacement placement,
    const utils::VisibilityInfo visib_info //
) {
  return new MemberFunction(mod, parent, name, returnTy, is_async, args,
                            has_variadic_args, true, returns_reference,
                            placement, visib_info);
}

MemberFunction *
MemberFunction::CreateDestructor(llvm::Module *mod, CoreType *parent,
                                 const utils::FilePlacement placement) {
  return new MemberFunction(
      mod, parent, "end", new VoidType(mod->getContext()), false,
      std::vector<Argument>(
          {Argument::CreateVariable("self", new PointerType(parent), 0)}),
      false, false, false, placement, utils::VisibilityInfo::pub());
}

bool MemberFunction::isVariationFunction() const {
  return isStatic
             ? false
             : function->getArg(0)->hasAttribute(llvm::Attribute::ReadOnly);
}

std::string MemberFunction::getFullName() const {
  return parent->getFullName() + ":" + name;
}

bool MemberFunction::isStaticFunction() const { return isStatic; }

llvm::CallInst *
MemberFunction::create_call(llvm::IRBuilder<> &builder,
                            llvm::ArrayRef<llvm::Value *> args //
) const {
  return builder.CreateCall(function->getFunctionType(), function, args,
                            getFullName(), nullptr);
}

bool MemberFunction::isMemberFunction() const { return true; }

} // namespace IR
} // namespace qat