#include "./function.hpp"
#include "../show.hpp"
#include "value.hpp"
#include <llvm/IR/Attributes.h>

namespace qat {
namespace IR {

Function::Function(llvm::Module *mod, std::string _parentName,
                   std::string _name, QatType *returnType, bool _is_async,
                   std::vector<Argument> _args, bool is_variable_arguments,
                   bool _returns_reference, utils::FilePlacement filePlacement,
                   utils::VisibilityInfo _visibility_info)
    : arguments(_args), name(_name), is_async(_is_async),
      returns_reference(_returns_reference), placement(filePlacement),
      visibility_info(_visibility_info), function(nullptr),
      Value(nullptr, false, Kind::pure) //
{
  std::vector<llvm::Type *> arg_types;
  SHOW("Separating arg types")
  for (auto arg : arguments) {
    arg_types.push_back(arg.getType()->getLLVMType());
  }
  SHOW("Types sorted")
  auto fnType = llvm::FunctionType::get(returnType->getLLVMType(), arg_types,
                                        is_variable_arguments);
  SHOW("Function type retrieved")
  function = llvm::Function::Create(
      fnType, llvm::GlobalValue::LinkageTypes::WeakAnyLinkage, 0u,
      (_parentName + ":" + _name), mod);
  SHOW("LLVM Function created")
  // If this is the main function, it can't recurse
  if ((_parentName == "") && (name == "main")) {
    function->setDoesNotRecurse();
  }
  utils::Visibility::set(function, visibility_info, mod->getContext());
  utils::PointerKind::set(mod->getContext(), function, _returns_reference);
  for (std::size_t i = 0; i < arguments.size(); i++) {
    utils::Variability::set(function->getArg(i),
                            arguments.at(i).get_variability());
  }
}

Function *Function::Create(llvm::Module *mod, const std::string parentName,
                           const std::string name, QatType *returnTy,
                           bool is_async, const std::vector<Argument> args,
                           const bool has_variadic_args,
                           const bool returns_reference,
                           const utils::FilePlacement placement,
                           const utils::VisibilityInfo visib_info) {
  std::vector<Argument> args_info;
  return new Function(mod, parentName, name, returnTy, is_async, args_info,
                      has_variadic_args, returns_reference, placement,
                      visib_info);
}

llvm::Function *Function::getLLVMFunction() { return function; }

bool Function::hasVariadicArgs() const {
  return function->getFunctionType()->isVarArg();
}

bool Function::isAsyncFunction() const { return is_async; }

std::string Function::argumentNameAt(unsigned int index) const {
  return arguments.at(index).get_name();
}

std::string Function::getName() const { return name; }

std::string Function::getFullName() const {
  return function->getParent()->getName().str() + ":" + name;
}

bool Function::isReturnTypeReference() const { return returns_reference; }

bool Function::isReturnTypePointer() const {
  return (!returns_reference) &&
         (function->getFunctionType()->getReturnType()->isPointerTy());
}

bool Function::isAccessible(const utils::RequesterInfo req_info) const {
  return utils::Visibility::isAccessible(visibility_info, req_info);
}

llvm::CallInst *Function::call(llvm::IRBuilder<> &builder,
                               llvm::ArrayRef<llvm::Value *> args,
                               utils::RequesterInfo req_info //
) const {
  if (isAccessible(req_info)) {
    return builder.CreateCall(function->getFunctionType(), function, args,
                              getFullName(), nullptr);
  } else {
    return nullptr;
  }
}

Block *Function::getEntryBlock() { return blocks.front(); }

Block *Function::addBlock(bool isSub) { return new Block(this, nullptr); }

Block *Function::getCurrentBlock() { return current; }

void Function::setCurrentBlock(Block *block) { current = block; }

utils::VisibilityInfo Function::getVisibility() const {
  return visibility_info;
}

bool Function::isMemberFunction() const { return false; }

} // namespace IR
} // namespace qat