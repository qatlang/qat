#include "./self.hpp"

namespace qat {
namespace AST {

Self::Self(utils::FilePlacement _filePlacement) : Expression(_filePlacement) {}

llvm::Value *Self::emit(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  auto parent = generator->builder.GetInsertBlock()->getParent();
  if (parent->getFunctionType()->getNumParams() > 1) {
    auto first_arg = parent->getArg(0);
    if (first_arg->getName().str() == "self") {
      auto result = generator->builder.CreateLoad(first_arg->getType(),
                                                  first_arg, false, "self");
      utils::Variability::propagate(generator->llvmContext, first_arg, result);
      return result;
    } else {
      auto fn_name = parent->getName().str();
      if (fn_name.find_first_of('\'', 0) != std::string::npos) {
        auto type_name = fn_name.substr(0, fn_name.find_first_of('\'', 0));
        auto type = llvm::StructType::getTypeByName(generator->llvmContext,
                                                    llvm::StringRef(type_name));
        if (!type) {
          generator->throw_error(
              "Function `" + parent->getName().str() +
                  "` is not a member function of any type, and hence is not "
                  "allowed to use '' expression!",
              file_placement);
        } else {
          generator->throw_error(
              "Function `" + parent->getName().str() +
                  "` is a static member of object `" + type_name +
                  "`, and hence is not allowed to use '' expression!",
              file_placement);
        }
      } else {
        generator->throw_error("Function `" + parent->getName().str() +
                                   "` is not a member of any object, and hence "
                                   "is not allowed to use '' expression!",
                               file_placement);
      }
    }
  } else {
    generator->throw_error("Function `" + parent->getName().str() +
                               "` is not a member of any object, and hence is "
                               "not allowed to use '' expression!",
                           file_placement);
  }
}

void Self::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "this";
  }
}

backend::JSON Self::toJSON() const {
  return backend::JSON()
      ._("nodeType", "selfExpression")
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat