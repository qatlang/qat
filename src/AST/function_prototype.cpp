#include "./function_prototype.hpp"
#include "../show.hpp"
#include <vector>

namespace qat {
namespace AST {

IR::Value *FunctionPrototype::emit(IR::Context *ctx) {
  IR::Function *function;
  std::vector<IR::QatType *> generatedTypes;
  if (ctx->mod->hasFunction(name)) {
    ctx->throw_error("A function named " + name +
                         " exists already in this scope",
                     file_placement);
  }
  SHOW("Generating types")
  int i = 1;
  for (auto arg : arguments) {
    auto genType = arg->getType()->emit(ctx);
    generatedTypes.push_back(genType);
    SHOW("Type number " << i)
    i++;
  }
  SHOW("Types generated")
  std::vector<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (std::size_t i = 0; i < generatedTypes.size(); i++) {
    args.push_back(arguments.at(i)->getType()->isVariable()
                       ? IR::Argument::CreateVariable(
                             arguments.at(i)->getName(),
                             arguments.at(i)->getType()->emit(ctx), i)
                       : IR::Argument::Create(arguments.at(i)->getName(),
                                              generatedTypes.at(i), i));
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  function = ctx->mod->createFunction(name, returnType->emit(ctx),
                                      returnType->isVariable(), isAsync, args,
                                      isVariadic, file_placement, visibility);
  SHOW("Function created!!")
  if ((linkageType == llvm::GlobalValue::ExternalLinkage) &&
      (utils::stringToCallingConv(callingConv) != 1024)) {
    // TODO - Set calling convention
    SHOW("Linkage set")
  }
  return function;
}

void FunctionPrototype::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if ((name == "main") && (arguments.size() == 2)) {
    file += "int main(int " + arguments.at(0)->getName() + ", char** " +
            arguments.at(1)->getName() + ")";
  } else {
    returnType->emitCPP(file, isHeader);
    file += (" " + name + "(");
    for (std::size_t i = 0; i < arguments.size(); i++) {
      if (arguments.at(i)->getType()) {
        arguments.at(i)->getType()->emitCPP(file, isHeader);
      } else {
        file += "auto ";
      }
      file += (" " + arguments.at(i)->getName());
      if (i != (arguments.size() - 1)) {
        file += ", ";
      }
    }
    file += ")";
  }
  if (isHeader) {
    file += ";\n";
  }
}

backend::JSON FunctionPrototype::toJSON() const {
  std::vector<backend::JSON> args;
  for (auto arg : arguments) {
    args.push_back(backend::JSON()
                       ._("name", arg->getName())
                       ._("type", arg->getType() ? arg->getType()->toJSON()
                                                 : backend::JSON())
                       ._("filePlacement", arg->getFilePlacement()));
  }
  std::cout << "Number of args " << arguments.size() << "\n";
  // FIXME - Add linkage to the JSON representation
  return backend::JSON()
      ._("nodeType", "functionPrototype")
      ._("name", name)
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJSON())
      ._("arguments", args)
      ._("isVariadic", isVariadic)
      ._("callingConvention", callingConv);
}

} // namespace AST
} // namespace qat