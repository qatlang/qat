#ifndef QAT_IR_MIX_TYPE_HPP
#define QAT_IR_MIX_TYPE_HPP

#include "../../utils/visibility.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class MixType : public QatType {
private:
  String                              name;
  QatModule                          *parent;
  Vec<Pair<String, Maybe<QatType *>>> subtypes;
  u64                                 maxSize = 0;
  bool                                isPack  = false;
  // NOLINTNEXTLINE(readability-magic-numbers)
  usize                 tagBitWidth = 1;
  utils::VisibilityInfo visibility;

  void findTagBitWidth(usize typeCount);

public:
  MixType(String name, QatModule *parent,
          Vec<Pair<String, Maybe<QatType *>>> subtypes, llvm::LLVMContext &ctx,
          bool isPacked, const utils::VisibilityInfo &visibility);

  useit String getName() const;
  useit String getFullName() const;
  useit usize  getIndexOfName(const String &name) const;
  useit Pair<bool, bool> hasSubTypeWithName(const String &sname) const;
  useit QatType         *getSubTypeWithName(const String &sname) const;
  useit usize            getSubTypeCount() const;
  useit QatModule       *getParent() const;
  useit bool             isPacked() const;
  useit usize            getTagBitwidth() const;
  useit u64              getDataBitwidth() const;
  useit bool  isAccessible(const utils::RequesterInfo &reqInfo) const;
  useit const utils::VisibilityInfo &getVisibility() const;
  useit String                       toString() const final;
  useit TypeKind                     typeKind() const final;
  // FIXME - Maybe remove this
  useit Json toJson() const final { return Json()._("name", name); }
  void       getMissingNames(Vec<String> &vals, Vec<String> &missing) const;
};

} // namespace qat::IR

#endif