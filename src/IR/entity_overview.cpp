#include "./entity_overview.hpp"
#include "./qat_module.hpp"

namespace qat::IR {

EntityOverview::EntityOverview(String _ovKind, Json _ovInfo, FileRange _ovRange)
    : ovKind(std::move(_ovKind)), ovInfo(std::move(_ovInfo)), ovRange(std::move(_ovRange)) {}

void EntityOverview::addMention(FileRange _range) { ovMentions.push_back(std::move(_range)); }

void EntityOverview::addBroughtMention(IR::QatModule* otherMod, const FileRange& fileRange) {
  ovBroughtMentions.push_back(Pair<IR::QatModule*, FileRange>(otherMod, fileRange));
}

Vec<Pair<QatModule*, FileRange>> const& EntityOverview::getBroughtMentions() const { return ovBroughtMentions; }

Json EntityOverview::overviewToJson() {
  if (!isOverviewUpdated) {
    updateOverview();
    isOverviewUpdated = true;
  }
  Vec<JsonValue> mentionsJson;
  for (const auto& fRange : ovMentions) {
    mentionsJson.push_back(fRange);
  }
  Vec<JsonValue> broughtMentionsJson;
  for (const auto& bMention : ovBroughtMentions) {
    broughtMentionsJson.push_back(Json()._("module", bMention.first->getID())._("range", bMention.second));
  }
  return Json()
      ._("kind", ovKind)
      ._("info", ovInfo)
      ._("origin", ovRange)
      ._("mentions", mentionsJson)
      ._("broughtMentions", broughtMentionsJson);
}

} // namespace qat::IR