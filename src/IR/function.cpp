#include "./function.hpp"
#include "../show.hpp"
#include "./qat_module.hpp"
#include "./types/function.hpp"
#include <vector>

namespace qat {
namespace IR {

Function::Function(QatModule *_mod, std::string _name,
                   QatType *returnType, bool _isRetTypeVariable, bool _is_async,
                   std::vector<Argument> _args, bool is_variable_arguments,
                   utils::FilePlacement filePlacement,
                   utils::VisibilityInfo _visibility_info)
    : arguments(_args), name(_name), isReturnValueVariable(_isRetTypeVariable),
      is_async(_is_async), mod(_mod), placement(filePlacement), has_variadic_args(is_variable_arguments),
      visibility_info(_visibility_info), Value(nullptr, false, Kind::pure) //
{
  std::vector<ArgumentType *> argTypes;
  for (auto arg : _args) {
    argTypes.push_back(
        new ArgumentType(arg.get_name(), arg.getType(), arg.get_variability()));
  }
  type = new FunctionType(returnType, _isRetTypeVariable, argTypes);
}

Function *Function::Create(QatModule *mod,
                           const std::string name, QatType *returnTy,
                           bool isReturnTypeVariable, bool is_async,
                           const std::vector<Argument> args,
                           const bool has_variadic_args,
                           const utils::FilePlacement placement,
                           const utils::VisibilityInfo visibilityInfo) {
  return new Function(mod, parentName, name, returnTy, isReturnTypeVariable,
                      is_async, args, has_variadic_args, placement,
                      visibilityInfo);
}

bool Function::hasVariadicArgs() const { return has_variadic_args; }

bool Function::isAsyncFunction() const { return is_async; }

std::string Function::argumentNameAt(unsigned int index) const {
  return arguments.at(index).get_name();
}

std::string Function::getName() const { return name; }

std::string Function::getFullName() const {
  return mod->getFullNameWithChild(name);
}

bool Function::isAccessible(const utils::RequesterInfo req_info) const {
  return utils::Visibility::isAccessible(visibility_info, req_info);
}

Block *Function::getEntryBlock() { return blocks.front(); }

Block *Function::addBlock(bool isSub) { return new Block(this, isSub ? getCurrentBlock() : nullptr); }

Block *Function::getCurrentBlock() { return current; }

void Function::setCurrentBlock(Block *block) { current = block; }

utils::VisibilityInfo Function::getVisibility() const {
  return visibility_info;
}

bool Function::isMemberFunction() const { return false; }

} // namespace IR
} // namespace qat