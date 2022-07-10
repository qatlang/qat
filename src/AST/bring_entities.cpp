#include "./bring_entities.hpp"
#include <vector>

namespace qat {
namespace AST {

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

backend::JSON BroughtGroup::toJSON() const {
  std::vector<backend::JSON> membes;
  for (auto mem : members) {
    mems.push_back(mem->toJSON());
  }
  return backend::JSON()._("parent", parent->toJSON())._("members", mems);
}

BringEntities::BringEntities(std::vector<BroughtGroup *> _entities,
                             utils::VisibilityInfo _visibility,
                             utils::FilePlacement _filePlacement)
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

backend::JSON BringEntities::toJSON() const {
  std::vector<backend::JSON> ents;
  for (auto ent : entities) {
    ents.push_back(ent->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "bringEntities")
      ._("entities", ents)
      ._("visibility", visibility)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat