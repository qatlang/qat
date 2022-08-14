#include "./named.hpp"

namespace qat::ast {

NamedType::NamedType(String _name, const bool _variable,
                     utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), name(std::move(_name)) {}

IR::QatType *NamedType::emit(IR::Context *ctx) {
  u32 relative = 0;
  u32 index    = 0;
  while (name.find("..:", index) == index) {
    relative++;
    index += 3;
  }
  // FIXME - Implement remaining logic
  // FIXME - Support type definitions and sum types too
  if (name.find("..:", index) != std::string::npos) {
    ctx->Error("Super token can be used only at the start", fileRange);
  } else {
    if (relative != 0) {
      if (ctx->getMod()->hasNthParent(relative)) {
        auto *mod     = ctx->getMod()->getNthParent(relative);
        auto  newName = name.substr(index);
        if (mod->hasCoreType(newName) || mod->hasBroughtCoreType(name) ||
            mod->hasAccessibleCoreTypeInImports(name, ctx->getReqInfo())
                .first) {
          return mod->getCoreType(name, ctx->getReqInfo());
        }
      } else {
        ctx->Error("The active scope does not have " +
                       std::to_string(relative) + " parents",
                   fileRange);
      }
    } else {
      auto *mod = ctx->getMod();
      if (mod->hasCoreType(name) || mod->hasBroughtCoreType(name) ||
          mod->hasAccessibleCoreTypeInImports(name, ctx->getReqInfo()).first) {
        return mod->getCoreType(name, ctx->getReqInfo());
      } else {
        ctx->Error("No type named " + name + " found in scope", fileRange);
      }
    }
  }
  return nullptr;
}

String NamedType::getName() const { return name; }

TypeKind NamedType::typeKind() const { return TypeKind::Float; }

nuo::Json NamedType::toJson() const {
  return nuo::Json()
      ._("typeKind", "named")
      ._("name", name)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String NamedType::toString() const {
  return (isVariable() ? "var " : "") + name;
}

} // namespace qat::ast