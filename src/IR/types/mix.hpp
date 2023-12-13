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
  u64                                    maxSize = 8u;
  bool                                   isPack  = false;

  usize           tagBitWidth = 1;
  Maybe<usize>    defaultVal;
  FileRange       fileRange;
  Maybe<MetaInfo> metaInfo;

  IR::OpaqueType* opaquedType = nullptr;

  void findTagBitWidth(usize typeCount);

public:
  MixType(Identifier name, IR::OpaqueType* opaquedTy, Vec<GenericParameter*> _generics, QatModule* parent,
          Vec<Pair<Identifier, Maybe<QatType*>>> subtypes, Maybe<usize> defaultVal, IR::Context* ctx, bool isPacked,
          const VisibilityInfo& visibility, FileRange fileRange, Maybe<MetaInfo> metaInfo);

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
  useit LinkNames        getLinkNames() const final;
  useit bool             isTypeSized() const final;

  useit bool isTriviallyCopyable() const final;
  useit bool isTriviallyMovable() const final;
  useit bool isCopyConstructible() const final;
  useit bool isCopyAssignable() const final;
  useit bool isMoveConstructible() const final;
  useit bool isMoveAssignable() const final;
  useit bool isDestructible() const final;

  void copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) final;
  void updateOverview() final;
  void getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const;
};

} // namespace qat::IR

#endif