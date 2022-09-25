#ifndef QAT_IR_CHOICE_HPP
#define QAT_IR_CHOICE_HPP

#include "../../utils/visibility.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class ChoiceType : public QatType {
private:
  String                name;
  QatModule            *parent;
  Vec<String>           fields;
  Maybe<Vec<i64>>       values;
  utils::VisibilityInfo visibility;

  mutable u64  bitwidth = 1;
  mutable bool hasNegative;

public:
  ChoiceType(String name, QatModule *parent, Vec<String> fields,
             Maybe<Vec<i64>> values, const utils::VisibilityInfo &visibility,
             llvm::LLVMContext &ctx);

  useit String     getName() const;
  useit String     getFullName() const;
  useit QatModule *getParent() const;
  useit bool       hasCustomValue() const;
  useit bool       hasField(const String &name) const;
  useit i64        getValueFor(const String &name) const;
  useit u64        getBitwidth() const;
  useit const utils::VisibilityInfo &getVisibility() const;
  useit TypeKind typeKind() const final { return TypeKind::choice; }
  useit nuo::Json toJson() const final { return nuo::Json(); }
  useit String    toString() const final { return getFullName(); }
  void            findBitwidthNormal() const;
  void            findBitwidthForValues() const;
  void getMissingNames(Vec<String> &vals, Vec<String> &missing) const;
};

} // namespace qat::IR

#endif