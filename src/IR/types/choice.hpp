#ifndef QAT_IR_CHOICE_HPP
#define QAT_IR_CHOICE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class QatModule;

class ChoiceType : public QatType, public EntityOverview {
private:
  Identifier                     name;
  QatModule*                     parent;
  Vec<Identifier>                fields;
  Maybe<Vec<llvm::ConstantInt*>> values;
  Maybe<IR::QatType*>            providedType;
  bool                           areValuesUnsigned = false;
  VisibilityInfo                 visibility;
  Maybe<usize>                   defaultVal;

  IR::Context* ctx;
  mutable u64  bitwidth = 1;

  FileRange fileRange;

public:
  ChoiceType(Identifier name, QatModule* parent, Vec<Identifier> fields, Maybe<Vec<llvm::ConstantInt*>> values,
             Maybe<IR::QatType*> providedType, bool areValuesUnsigned, Maybe<usize> defaultVal,
             const VisibilityInfo& visibility, IR::Context* ctx, FileRange fileRange);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit QatModule* getParent() const;
  useit bool       hasCustomValue() const;
  useit bool       hasNegativeValues() const;
  useit bool       hasDefault() const;
  useit bool       hasField(const String& name) const;
  useit llvm::ConstantInt* getValueFor(const String& name) const;
  useit llvm::ConstantInt*    getDefault() const;
  useit u64                   getBitwidth() const;
  useit TypeKind              typeKind() const final { return TypeKind::choice; }
  useit String                toString() const final;
  useit const VisibilityInfo& getVisibility() const;
  void                        findBitwidthNormal() const;
  void                        findBitwidthForValues() const;
  void                        getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const;
  void                        updateOverview() final;
  useit bool                  isTypeSized() const final;
  useit bool                  isTriviallyCopyable() const final { return true; }
  useit bool                  isTriviallyMovable() const final { return true; }
  useit bool                  canBePrerunGeneric() const final;
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif