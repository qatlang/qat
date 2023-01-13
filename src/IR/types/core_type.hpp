#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
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
}

namespace qat::IR {

/**
 *  This represents a core type in the language
 *
 */
class CoreType final : public ExpandedType, public EntityOverview {
  friend class MemberFunction;

public:
  class Member final : public EntityOverview {
  public:
    Member(Identifier _name, QatType* _type, bool _variability, const utils::VisibilityInfo& _visibility)
        : EntityOverview("coreTypeMember",
                         Json()
                             ._("name", _name.value)
                             ._("type", _type->toString())
                             ._("typeID", _type->getID())
                             ._("isVariable", _variability)
                             ._("visibility", _visibility),
                         _name.range),
          name(std::move(_name)), type(_type), visibility(_visibility), variability(_variability) {}

    ~Member() final = default;

    Identifier            name;
    QatType*              type;
    utils::VisibilityInfo visibility;
    bool                  variability;
  };

private:
  Vec<Member*>       members;
  Vec<StaticMember*> staticMembers;

  // TODO - Add support for extension functions

public:
  CoreType(QatModule* mod, Identifier _name, Vec<Member*> _members, const utils::VisibilityInfo& _visibility,
           llvm::LLVMContext& ctx);

  ~CoreType() final;

  useit Maybe<usize> getIndexOf(const String& member) const;
  useit bool         hasMember(const String& member) const;
  useit Member*      getMember(const String& name) const;
  useit u64          getMemberCount() const;
  useit Member*      getMemberAt(u64 index);
  useit String       getMemberNameAt(u64 index) const;
  useit QatType*     getTypeOfMember(const String& member) const;
  useit Vec<Member*>& getMembers();
  useit bool          hasStatic(const String& name) const;

  useit Json     toJson() const override;
  useit TypeKind typeKind() const override;
  useit String   toString() const override;
  void           addStaticMember(const Identifier& name, QatType* type, bool variability, Value* initial,
                                 const utils::VisibilityInfo& visibility, llvm::LLVMContext& ctx);
  void           updateOverview() final;
};

class TemplateCoreType : public Uniq, public EntityOverview {
private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> templates;
  ast::DefineCoreType*           defineCoreType;
  QatModule*                     parent;
  utils::VisibilityInfo          visibility;

  mutable Vec<TemplateVariant<CoreType>> variants;

public:
  TemplateCoreType(Identifier name, Vec<ast::GenericAbstractType*> templates, ast::DefineCoreType* defineCoreType,
                   QatModule* parent, const utils::VisibilityInfo& visibInfo);

  ~TemplateCoreType() = default;

  useit Identifier getName() const;
  useit utils::VisibilityInfo getVisibility() const;
  useit usize                 getTypeCount() const;
  useit bool                  allTypesHaveDefaults() const;
  useit Vec<IR::QatType*> getDefaults(IR::Context* ctx) const;
  useit usize             getVariantCount() const;
  useit QatModule*        getModule() const;
  useit CoreType*         fillTemplates(Vec<QatType*>& templates, IR::Context* ctx, FileRange range);
};

} // namespace qat::IR

#endif