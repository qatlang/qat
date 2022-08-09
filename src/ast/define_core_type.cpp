#include "./define_core_type.hpp"
#include "../IR/types/core_type.hpp"

namespace qat::ast {

DefineCoreType::Member::Member( //
    QatType *_type, String _name, bool _variability,
    const utils::VisibilityInfo &_visibility, utils::FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability),
      visibility(_visibility), fileRange(std::move(_fileRange)) {}

nuo::Json DefineCoreType::Member::toJson() const {
  return nuo::Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

DefineCoreType::StaticMember::StaticMember(
    QatType *_type, String _name, bool _variability, Expression *_value,
    const utils::VisibilityInfo &_visibility, utils::FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability),
      value(_value), visibility(_visibility), fileRange(std::move(_fileRange)) {
}

nuo::Json DefineCoreType::StaticMember::toJson() const {
  return nuo::Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

DefineCoreType::DefineCoreType(String _name, Vec<Member *> _members,
                               Vec<StaticMember *>          _staticMembers,
                               const utils::VisibilityInfo &_visibility,
                               utils::FileRange _fileRange, bool _isPacked)
    : Node(std::move(_fileRange)), name(std::move(_name)), isPacked(_isPacked),
      members(std::move(_members)), staticMembers(std::move(_staticMembers)),
      visibility(_visibility) {}

IR::Value *DefineCoreType::emit(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if (!IR::QatType::checkTypeExists(mod->getFullNameWithChild(name))) {
    Vec<IR::CoreType::Member *> mems;
    for (auto *mem : members) {
      mems.push_back(new IR::CoreType::Member(
          mem->name, mem->type->emit(ctx), mem->variability, mem->visibility));
    }
    auto *coreType = new IR::CoreType(mod, name, mems, visibility, ctx->llctx);
    for (auto *stm : staticMembers) {
      coreType->addStaticMember(stm->name, stm->type->emit(ctx),
                                stm->variability,
                                stm->value ? stm->value->emit(ctx) : nullptr,
                                stm->visibility, ctx->llctx);
    }
    return nullptr; // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
  } else {
    ctx->Error(
        "Type " + mod->getFullNameWithChild(name) +
            " exists in this scope already. Please change name of this type or "
            "check the codebase",
        fileRange);
  }
}

nuo::Json DefineCoreType::toJson() const {
  Vec<nuo::JsonValue> membersJsonValue;
  Vec<nuo::JsonValue> staticMembersJsonValue;
  for (auto *mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  for (auto *mem : staticMembers) {
    staticMembersJsonValue.emplace_back(mem->toJson());
  }
  return nuo::Json()
      ._("nodeType", "defineCoreType")
      ._("members", membersJsonValue)
      ._("staticMembers", staticMembersJsonValue)
      ._("isPacked", isPacked)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

} // namespace qat::ast