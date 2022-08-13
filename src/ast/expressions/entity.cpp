#include <utility>

#include "./entity.hpp"

namespace qat::ast {

Entity::Entity(String _name, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), name(std::move(_name)) {}

IR::Value *Entity::emit(IR::Context *ctx) {
  auto *fun = ctx->fn;
  SHOW("Entity name is " << name)
  if (name.find("..:") == std::string::npos) {
    if (fun) {
      SHOW("Found function")
      if (fun->getBlock()->hasValue(name)) {
        SHOW("Found local value: " << name)
        auto *local  = fun->getBlock()->getValue(name);
        auto *alloca = local->getAlloca();
        if (getExpectedKind() == ExpressionKind::assignable) {
          if (local->isVariable()) {
            return new IR::Value(alloca, local->getType(), local->isVariable(),
                                 IR::Nature::assignable);
          } else {
            ctx->Error(ctx->highlightError(name) +
                           " is not a variable and is not assignable",
                       fileRange);
          }
        } else {
          return new IR::Value(alloca, local->getType(), local->isVariable(),
                               IR::Nature::temporary);
        }
      } else {
        SHOW("No local value with name: " << name)
        // Checking arguments
        auto argTypes = fun->getType()->asFunction()->getArgumentTypes();
        for (usize i = 0; i < argTypes.size(); i++) {
          if (argTypes.at(i)->getName() == name) {
            if (getExpectedKind() == ExpressionKind::assignable) {
              ctx->Error("Argument " + name +
                             " is not assignable. If the argument should be "
                             "reassigned in the function, make it var",
                         fileRange);
            } else {
              return new IR::Value(
                  ((llvm::Function *)(fun->getLLVM()))->getArg(i),
                  argTypes.at(i)->getType(), false, IR::Nature::pure);
            }
          }
        }
        ctx->Error("No value named " + ctx->highlightError(name) + " found",
                   fileRange);
      }
    }
  }
}

nuo::Json Entity::toJson() const {
  return nuo::Json()
      ._("nodeType", "entity")
      ._("name", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast