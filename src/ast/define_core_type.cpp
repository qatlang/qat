#include "./define_core_type.hpp"
#include "../IR/types/core_type.hpp"

namespace qat::ast {

DefineCoreType::Member::Member( //
    QatType *_type, std::string _name, bool _variability,
    utils::VisibilityInfo _visibility, utils::FileRange _fileRange)
    : type(_type), name(_name), variability(_variability),
      visibility(_visibility), fileRange(_fileRange) {}

DefineCoreType::StaticMember::StaticMember( //
    QatType *_type, std::string _name, bool _variability, Expression *_value,
    utils::VisibilityInfo _visibility, utils::FileRange _fileRange)
    : type(_type), name(_name), variability(_variability), value(_value),
      visibility(_visibility), fileRange(_fileRange) {}

DefineCoreType::DefineCoreType( //
    std::string _name, std::vector<Member *> _members,
    std::vector<StaticMember *> _staticMembers,
    utils::VisibilityInfo _visibility, utils::FileRange _fileRange,
    bool _isPacked)
    : name(_name), isPacked(_isPacked), members(_members),
      visibility(_visibility), Node(_fileRange) {}

IR::Value *DefineCoreType::emit(IR::Context *ctx) {
  auto mod = ctx->getActive();
  if (!IR::QatType::checkTypeExists(mod->getFullNameWithChild(name))) {
    std::vector<IR::CoreType::Member *> mems;
    for (auto mem : members) {
      mems.push_back(new IR::CoreType::Member(
          mem->name, mem->type->emit(ctx), mem->variability, mem->visibility));
    }
    auto coreType = new IR::CoreType(mod, name, mems, {}, visibility);
    for (auto st : staticMembers) {
      coreType->addStaticMember(st->name, st->type->emit(ctx), st->variability,
                                st->value ? st->value->emit(ctx) : nullptr,
                                st->visibility);
    }
    return nullptr;
  } else {
    ctx->throw_error("Type " + mod->getFullNameWithChild(name) +
                         " exists already. Please change name of this type or "
                         "check the codebase",
                     fileRange);
  }
}

nuo::Json DefineCoreType::toJson() const {
  std::vector<nuo::JsonValue> mems;
  std::vector<nuo::JsonValue> staticMems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  for (auto mem : staticMembers) {
    staticMems.push_back(mem->toJson());
  }
  return nuo::Json()
      ._("nodeType", "defineCoreType")
      ._("members", mems)
      ._("staticMembers", staticMems)
      ._("isPacked", isPacked)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

} // namespace qat::ast