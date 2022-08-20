#include "./entity.hpp"
#include <utility>

namespace qat::ast {

Entity::Entity(u32 _relative, String _name, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), name(std::move(_name)),
      relative(_relative) {}

IR::Value *Entity::emit(IR::Context *ctx) {
  auto *fun = ctx->fn;
  SHOW("Entity name is " << name)
  if (relative == 0) {
    if (fun) {
      if (fun->getBlock()->hasValue(name)) {
        SHOW("Found local value: " << name)
        auto *local  = fun->getBlock()->getValue(name);
        auto *alloca = local->getAlloca();
        if (getExpectedKind() == ExpressionKind::assignable) {
          if (local->isVariable() ||
              (local->getType()->isReference() &&
               local->getType()->asReference()->isSubtypeVariable())) {
            auto *val = new IR::Value(alloca, local->getType(), true,
                                      IR::Nature::assignable);
            val->setIsLocalToFn(true);
            return val;
          } else {
            ctx->Error(ctx->highlightError(name) +
                           " is not a variable and is not assignable",
                       fileRange);
          }
        } else {
          auto *val = new IR::Value(alloca, local->getType(),
                                    local->isVariable(), IR::Nature::temporary);
          val->setIsLocalToFn(true);
          return val;
        }
      } else {
        SHOW("No local value with name: " << name)
        // Checking arguments
        // Currently this is unnecessary, since all arguments are allocated as
        // local values first
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
        // Checking functions
        auto *mod = ctx->getMod();
        if (mod->hasFunction(name)) {
          return mod->getFunction(name, ctx->getReqInfo());
        }
        ctx->Error("No value named " + ctx->highlightError(name) + " found",
                   fileRange);
      }
    }
  } else {
    if (ctx->getMod()->hasNthParent(relative)) {
      auto *mod = ctx->getMod()->getNthParent(relative);
      // TODO - Implement remaining logic
    } else {
      ctx->Error("The current scope do not have " +
                     (relative == 1 ? "a" : std::to_string(relative)) +
                     " parent" + (relative == 1 ? "" : "s") +
                     ". Relative mentions of identifiers cannot be used here. "
                     "Please check the logic.",
                 fileRange);
    }
  }
}

nuo::Json Entity::toJson() const {
  return nuo::Json()
      ._("nodeType", "entity")
      ._("name", name)
      ._("relative", relative)
      ._("fileRange", fileRange);
}

} // namespace qat::ast