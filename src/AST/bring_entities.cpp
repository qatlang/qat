#include "./bring_entities.hpp"
#include <vector>

namespace qat::AST {

BroughtGroup::BroughtGroup(StringLiteral *_parent)
    : parent(_parent), members({}) {}

BroughtGroup::BroughtGroup(StringLiteral *_parent,
                           std::vector<StringLiteral *> _members)
    : parent(_parent), members(_members) {}

StringLiteral *BroughtGroup::get_parent() const { return parent; }

std::vector<StringLiteral *> BroughtGroup::get_members() const {
  return members;
}

bool BroughtGroup::is_all_brought() const { return members.empty(); }

nuo::Json BroughtGroup::toJson() const {
  std::vector<nuo::JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()._("parent", parent->toJson())._("members", mems);
}

BringEntities::BringEntities(std::vector<BroughtGroup *> _entities,
                             utils::VisibilityInfo _visibility,
                             utils::FileRange _filePlacement)
    : entities(_entities), visibility(_visibility), Node(_filePlacement) {}

IR::Value *BringEntities::emit(IR::Context *ctx) {
  // FIXME - Implement this
  return nullptr;
}

void BringEntities::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isHeader) {
    for (auto bGroup : entities) {
      if (bGroup->is_all_brought()) {
        file.addSingleLineComment("Brought " +
                                  bGroup->get_parent()->get_value());
      } else {
        std::string val =
            "Brought members " + bGroup->get_parent()->get_value() + " { ";
        auto mems = bGroup->get_members();
        for (std::size_t i = 0; i < mems.size(); i++) {
          val += mems.at(i)->get_value();
          if (i != (mems.size() - 1)) {
            val += ", ";
          }
        }
        val += " } ";
        file.addSingleLineComment(val);
      }
    }
  }
}

nuo::Json BringEntities::toJson() const {
  std::vector<nuo::JsonValue> ents;
  for (auto ent : entities) {
    ents.push_back(ent->toJson());
  }
  return nuo::Json()
      ._("nodeType", "bringEntities")
      ._("entities", ents)
      ._("visibility", visibility)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST