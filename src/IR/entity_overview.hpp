#ifndef QAT_IR_ENTITY_OVERVIEW_HPP
#define QAT_IR_ENTITY_OVERVIEW_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"

namespace qat::ir {

class Mod;

class EntityOverview {
	friend class StructType;
	friend class MixType;

  protected:
	String            ovKind;
	Json              ovInfo;
	FileRangePtr      ovRange;
	Vec<FileRangePtr> ovMentions;
	bool              isOverviewUpdated = false;

	Vec<Pair<Mod*, FileRangePtr>> ovBroughtMentions;

  public:
	EntityOverview(String _ovKind, Json _ovInfo, FileRangePtr _ovRange);

	virtual ~EntityOverview() = default;

	void add_mention(FileRangePtr _range);
	void add_bring_mention(Mod* module, FileRangePtr range);

	Vec<Pair<Mod*, FileRangePtr>> const& get_brought_mentions() const;

	virtual void update_overview() {}

	Json overviewToJson();
};

} // namespace qat::ir

#endif
