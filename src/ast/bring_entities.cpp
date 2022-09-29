#include "./bring_entities.hpp"
#include <utility>
#include <vector>

namespace qat::ast {

BroughtGroup::BroughtGroup(String _parent, utils::FileRange _fileRange)
    : parent(std::move(_parent)), fileRange(std::move(_fileRange)) {}

BroughtGroup::BroughtGroup(String _parent, Vec<String> _members,
                           utils::FileRange _fileRange)
    : parent(std::move(_parent)), members(std::move(_members)),
      fileRange(std::move(_fileRange)) {}

String BroughtGroup::getParent() const { return parent; }

Vec<String> BroughtGroup::getMembers() const { return members; }

bool BroughtGroup::isAllBrought() const { return members.empty(); }

Json BroughtGroup::toJson() const {
  Vec<JsonValue> membersJson;
  for (auto const &mem : members) {
    membersJson.emplace_back(mem);
  }
  return Json()
      ._("parent", parent)
      ._("members", membersJson)
      ._("fileRange", fileRange);
}

BringEntities::BringEntities(Vec<BroughtGroup *>          _entities,
                             const utils::VisibilityInfo &_visibility,
                             utils::FileRange             _fileRange)
    : entities(std::move(_entities)), visibility(_visibility),
      Node(std::move(_fileRange)) {}

IR::Value *BringEntities::emit(IR::Context *ctx) {
  // FIXME - Implement this
  return nullptr;
}

Json BringEntities::toJson() const {
  Vec<JsonValue> entitiesJson;
  for (auto *ent : entities) {
    entitiesJson.emplace_back(ent->toJson());
  }
  return Json()
      ._("nodeType", "bringEntities")
      ._("entities", entitiesJson)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

} // namespace qat::ast