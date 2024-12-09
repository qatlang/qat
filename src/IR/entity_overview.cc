#include "./entity_overview.hpp"
#include "./qat_module.hpp"

namespace qat::ir {

EntityOverview::EntityOverview(String _ovKind, Json _ovInfo, FileRange _ovRange)
    : ovKind(std::move(_ovKind)), ovInfo(std::move(_ovInfo)), ovRange(std::move(_ovRange)) {}

void EntityOverview::add_mention(FileRange _range) { ovMentions.push_back(std::move(_range)); }

void EntityOverview::add_bring_mention(ir::Mod* otherMod, const FileRange& fileRange) {
	ovBroughtMentions.push_back(Pair<ir::Mod*, FileRange>(otherMod, fileRange));
}

Vec<Pair<Mod*, FileRange>> const& EntityOverview::get_brought_mentions() const { return ovBroughtMentions; }

Json EntityOverview::overviewToJson() {
	if (!isOverviewUpdated) {
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
	    ._("origin", ovRange)
	    ._("mentions", mentionsJson)
	    ._("broughtMentions", broughtMentionsJson);
}

} // namespace qat::ir
