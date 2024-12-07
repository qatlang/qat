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
	String		   ovKind;
	Json		   ovInfo;
	FileRange	   ovRange;
	Vec<FileRange> ovMentions;
	bool		   isOverviewUpdated = false;

	Vec<Pair<Mod*, FileRange>> ovBroughtMentions;

  public:
	EntityOverview(String _ovKind, Json _ovInfo, FileRange _ovRange);

	virtual ~EntityOverview() = default;

	void add_mention(FileRange _range);
	void add_bring_mention(Mod* module, const FileRange& range);

	Vec<Pair<Mod*, FileRange>> const& get_brought_mentions() const;

	virtual void update_overview() {}
	Json		 overviewToJson();
};

} // namespace qat::ir

#endif
