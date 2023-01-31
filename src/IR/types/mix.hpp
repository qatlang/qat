#ifndef QAT_IR_MIX_TYPE_HPP
#define QAT_IR_MIX_TYPE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../generics.hpp"
#include "./expanded_type.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class MixType : public ExpandedType, public EntityOverview {
private:
  Vec<Pair<Identifier, Maybe<QatType*>>> subtypes;
  u64                                    maxSize = 0;
  bool                                   isPack  = false;
  // NOLINTNEXTLINE(readability-magic-numbers)
  usize        tagBitWidth = 1;
  Maybe<usize> defaultVal;
  FileRange    fileRange;

  void findTagBitWidth(usize typeCount);

public:
  MixType(Identifier name, Vec<GenericType*> _generics, QatModule* parent,
          Vec<Pair<Identifier, Maybe<QatType*>>> subtypes, Maybe<usize> defaultVal, IR::Context* ctx, bool isPacked,
          const utils::VisibilityInfo& visibility, FileRange fileRange);

  useit usize getIndexOfName(const String& name) const;
  useit Pair<bool, bool> hasSubTypeWithName(const String& sname) const;
  useit QatType*         getSubTypeWithName(const String& sname) const;
  useit bool             hasDefault() const;
  useit usize            getDefault() const;
  useit usize            getSubTypeCount() const;
  useit bool             isPacked() const;
  useit usize            getTagBitwidth() const;
  useit u64              getDataBitwidth() const;
  useit FileRange        getFileRange() const;
  useit String           toString() const final;
  useit TypeKind         typeKind() const final;
  // FIXME - Maybe remove this
  useit Json toJson() const final { return Json()._("name", name); }
  void       updateOverview() final;
  void       getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const;
};

} // namespace qat::IR

#endif