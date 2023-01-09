#ifndef QAT_IR_ENTITY_OVERVIEW_HPP
#define QAT_IR_ENTITY_OVERVIEW_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"

namespace qat::IR {

class EntityOverview {
protected:
  String         ovKind;
  Json           ovInfo;
  FileRange      ovRange;
  Vec<FileRange> ovMentions;
  bool           isOverviewUpdated = false;

public:
  EntityOverview(String _ovKind, Json _ovInfo, FileRange _ovRange);

  virtual ~EntityOverview() = default;

  void addMention(FileRange _range);

  virtual void updateOverview() {}

  Json overviewToJson();
};

} // namespace qat::IR

#endif