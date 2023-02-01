#ifndef QAT_IR_CHOICE_HPP
#define QAT_IR_CHOICE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class ChoiceType : public QatType, public EntityOverview {
private:
  Identifier            name;
  QatModule*            parent;
  Vec<Identifier>       fields;
  Maybe<Vec<i64>>       values;
  utils::VisibilityInfo visibility;
  Maybe<usize>          defaultVal;

  mutable u64  bitwidth = 1;
  mutable bool hasNegative;

  FileRange fileRange;

public:
  ChoiceType(Identifier name, QatModule* parent, Vec<Identifier> fields, Maybe<Vec<i64>> values,
             Maybe<usize> defaultVal, const utils::VisibilityInfo& visibility, llvm::LLVMContext& ctx,
             FileRange fileRange);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit QatModule* getParent() const;
  useit bool       hasCustomValue() const;
  useit bool       hasNegativeValues() const;
  useit bool       hasDefault() const;
  useit bool       hasField(const String& name) const;
  useit i64        getValueFor(const String& name) const;
  useit i64        getDefault() const;
  useit u64        getBitwidth() const;
  useit TypeKind   typeKind() const final { return TypeKind::choice; }
  useit Json       toJson() const final { return {}; }
  useit String     toString() const final;
  useit const utils::VisibilityInfo& getVisibility() const;
  void                               findBitwidthNormal() const;
  void                               findBitwidthForValues() const;
  void                               getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const;
  void                               updateOverview() final;

  useit bool canBeConstGeneric() const final;
  useit Maybe<String> toConstGenericString(IR::ConstantValue* val) const final;
  useit Maybe<bool> equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const final;
};

} // namespace qat::IR

#endif