#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../generics.hpp"
#include "../member_function.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
#include "expanded_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <utility>
#include <vector>

namespace qat::ast {
class DefineCoreType;
class Expression;
} // namespace qat::ast

namespace qat::IR {

/**
 *  This represents a core type in the language
 *
 */
class CoreType final : public ExpandedType, public EntityOverview {
  friend class MemberFunction;
  friend class GenericParameter;

public:
  class Member final : public EntityOverview, public Uniq {
  public:
    Member(Identifier _name, QatType* _type, bool _variability, Maybe<ast::Expression*> _defVal,
           const VisibilityInfo& _visibility)
        : EntityOverview("coreTypeMember",
                         Json()
                             ._("name", _name.value)
                             ._("type", _type->toString())
                             ._("typeID", _type->getID())
                             ._("isVariable", _variability)
                             ._("visibility", _visibility),
                         _name.range),
          name(std::move(_name)), type(_type), defaultValue(_defVal), visibility(_visibility),
          variability(_variability) {}

    ~Member() final = default;

    Identifier              name;
    QatType*                type;
    Maybe<ast::Expression*> defaultValue;
    VisibilityInfo          visibility;
    bool                    variability;
  };

private:
  IR::OpaqueType*    opaquedType = nullptr;
  Vec<Member*>       members;
  Vec<StaticMember*> staticMembers;
  Maybe<MetaInfo>    metaInfo;

public:
  CoreType(QatModule* mod, Identifier _name, Vec<GenericParameter*> _generics, IR::OpaqueType* _opaqued,
           Vec<Member*> _members, const VisibilityInfo& _visibility, llvm::LLVMContext& llctx, Maybe<MetaInfo> metaInfo,
           bool isPacked);

  ~CoreType() final;

  useit Maybe<usize> getIndexOf(const String& member) const;
  useit bool         hasMember(const String& member) const;
  useit Member*      getMember(const String& name) const;
  useit u64          getMemberCount() const;
  useit Member*      getMemberAt(u64 index);
  useit usize        getMemberIndex(String const& name) const;
  useit String       getMemberNameAt(u64 index) const;
  useit QatType*     getTypeOfMember(const String& member) const;
  useit Vec<Member*>& getMembers();
  useit bool          hasStatic(const String& name) const;
  useit bool          isTypeSized() const final;

  useit bool isTriviallyCopyable() const final;
  useit bool isTriviallyMovable() const final;
  useit bool isCopyConstructible() const final;
  useit bool isCopyAssignable() const final;
  useit bool isMoveConstructible() const final;
  useit bool isMoveAssignable() const final;
  useit bool isDestructible() const final;

  useit inline bool canBePrerun() const final {
    for (auto* mem : members) {
      if (!mem->type->canBePrerun()) {
        return false;
      }
    }
    return true;
  }

  useit inline bool canBePrerunGeneric() const final {
    for (auto* mem : members) {
      if (!mem->type->canBePrerunGeneric()) {
        return false;
      }
    }
    return true;
  }

  useit inline Maybe<String> toPrerunGenericString(IR::PrerunValue* value) const final {
    if (canBePrerunGeneric()) {
      auto   valConst = value->getLLVMConstant();
      String result(getFullName());
      result.append("{ ");
      for (usize i = 0; i < members.size(); i++) {
        result.append(
            members[i]
                ->type->toPrerunGenericString(new IR::PrerunValue(valConst->getAggregateElement(i), members[i]->type))
                .value());
        if (i != (members.size() - 1)) {
          result.append(", ");
        }
      }
      result.append(" }");
      return result;
    }
    return None;
  }

  void copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) final;

  useit LinkNames getLinkNames() const final;
  useit TypeKind  typeKind() const override;
  useit String    toString() const override;
  void            addStaticMember(const Identifier& name, QatType* type, bool variability, Value* initial,
                                  const VisibilityInfo& visibility, llvm::LLVMContext& llctx);
  void            updateOverview() final;
};

class GenericCoreType : public Uniq, public EntityOverview {
  friend ast::DefineCoreType;

private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> generics;
  ast::DefineCoreType*           defineCoreType;
  QatModule*                     parent;
  VisibilityInfo                 visibility;
  Maybe<ast::PrerunExpression*>  constraint;

  Vec<String> variantNames;

  mutable Vec<GenericVariant<CoreType>>     variants;
  mutable Deque<GenericVariant<OpaqueType>> opaqueVariants;

public:
  GenericCoreType(Identifier name, Vec<ast::GenericAbstractType*> generics, Maybe<ast::PrerunExpression*> _constraint,
                  ast::DefineCoreType* defineCoreType, QatModule* parent, const VisibilityInfo& visibInfo);

  ~GenericCoreType() = default;

  useit Identifier getName() const;
  useit usize      getTypeCount() const;
  useit bool       allTypesHaveDefaults() const;
  useit usize      getVariantCount() const;
  useit QatModule* getModule() const;
  useit QatType*   fillGenerics(Vec<IR::GenericToFill*>& types, IR::Context* ctx, FileRange range);

  useit ast::GenericAbstractType* getGenericAt(usize index) const;
  useit VisibilityInfo            getVisibility() const;
};

} // namespace qat::IR

#endif