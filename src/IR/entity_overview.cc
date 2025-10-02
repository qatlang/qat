#include "./entity_overview.hpp"
#include "./qat_module.hpp"

namespace qat::ir {

EntityOverview::EntityOverview(String _ovKind, Json _ovInfo, FileRangePtr _ovRange)
    : ovKind(std::move(_ovKind)), ovInfo(std::move(_ovInfo)), ovRange(_ovRange) {}

void EntityOverview::add_mention(FileRangePtr _range) { ovMentions.push_back(_range); }

void EntityOverview::add_bring_mention(ir::Mod* otherMod, FileRangePtr fileRange) {
	ovBroughtMentions.push_back(Pair<ir::Mod*, FileRangePtr>(otherMod, fileRange));
}

Vec<Pair<Mod*, FileRangePtr>> const& EntityOverview::get_brought_mentions() const { return ovBroughtMentions; }

Json EntityOverview::overviewToJson() {
	if (not isOverviewUpdated) {
		update_overview();
		isOverviewUpdated = true;
	}
	Vec<JsonValue> mentionsJson;
	for (const auto& fRange : ovMentions) {
		mentionsJson.push_back(fRange);
	}
	Vec<JsonValue> broughtMentionsJson;
	for (const auto& bMention : ovBroughtMentions) {
		broughtMentionsJson.push_back(Json()._("module", bMention.first->get_id())._("range", bMention.second));
	}
	return Json()
	    ._("kind", ovKind)
	    ._("info", ovInfo)
	    ._("origin", ovRange ? ovRange : JsonValue())
	    ._("mentions", mentionsJson)
	    ._("broughtMentions", broughtMentionsJson);
}

} // namespace qat::ir
