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

DefineCoreType::DefineCoreType(String                       _name,
                               const utils::VisibilityInfo &_visibility,
                               utils::FileRange _fileRange, bool _isPacked)
    : Node(std::move(_fileRange)), name(std::move(_name)), isPacked(_isPacked),
      visibility(_visibility){SHOW("Created core type " + name)}

          IR::Value
          * DefineCoreType::emit(IR::Context * ctx) {
  auto *mod = ctx->getMod();
  if (!mod->hasCoreType(name) && !mod->hasFunction(name) &&
      !mod->hasGlobalEntity(name) && !mod->hasBox(name)) {
    SHOW("Creating IR for CoreType members")
    Vec<IR::CoreType::Member *> mems;
    for (auto *mem : members) {
      mems.push_back(new IR::CoreType::Member(
          mem->name, mem->type->emit(ctx), mem->variability, mem->visibility));
    }
    SHOW("Emitted IR for all members")
    auto *coreType = new IR::CoreType(mod, name, mems, visibility, ctx->llctx);
    SHOW("Created core type in IR")
    for (auto *stm : staticMembers) {
      coreType->addStaticMember(stm->name, stm->type->emit(ctx),
                                stm->variability,
                                stm->value ? stm->value->emit(ctx) : nullptr,
                                stm->visibility, ctx->llctx);
    }
    return nullptr; // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
  } else {
    // TODO - Check type definitions
    if (mod->hasCoreType(name)) {
      ctx->Error(name + " is the name of an existing type in this scope. "
                        "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(name + " is the name of an existing function in this scope. "
                        "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(name +
                     " is the name of an existing global value in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(name + " is the name of an existing box in this scope. "
                        "Please change name of this type or check the codebase",
                 fileRange);
    }
  }
}

void DefineCoreType::addMember(Member *mem) {
  SHOW("Added core type member: " + mem->name)
  members.push_back(mem);
}

void DefineCoreType::addStaticMember(StaticMember *stm) {
  SHOW("Added core type static member: " + stm->name)
  staticMembers.push_back(stm);
}

void DefineCoreType::addMemberDefinition(MemberDefinition *mdef) {
  memberDefinitions.push_back(mdef);
}
void DefineCoreType::addConvertorDefinition(ConvertorDefinition *cdef) {
  convertorDefinitions.push_back(cdef);
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