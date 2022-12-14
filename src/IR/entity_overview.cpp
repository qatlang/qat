#include "./entity_overview.hpp"

namespace qat::IR {

EntityOverview::EntityOverview(String _ovKind, Json _ovInfo, FileRange _ovRange)
    : ovKind(std::move(_ovKind)), ovInfo(std::move(_ovInfo)), ovRange(std::move(_ovRange)) {}

void EntityOverview::addMention(FileRange _range) { ovMentions.push_back(std::move(_range)); }

Json EntityOverview::overviewToJson() {
  if (!isOverviewUpdated) {
    updateOverview();
    isOverviewUpdated = true;
  }
  Vec<JsonValue> mentionsJson;
  for (const auto& fRange : ovMentions) {
    mentionsJson.push_back(fRange);
  }
  return Json()._("kind", ovKind)._("info", ovInfo)._("origin", ovRange)._("mentions", mentionsJson);
}

} // namespace qat::IR