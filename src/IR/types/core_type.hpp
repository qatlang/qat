#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/visibility.hpp"
#include "../member_function.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
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
class CoreType : public QatType {
  friend class MemberFunction;

public:
  class Member {
  public:
    Member(String _name, QatType* _type, bool _variability, const utils::VisibilityInfo& _visibility)
        : name(std::move(_name)), type(_type), visibility(_visibility), variability(_variability) {}

    String                name;
    QatType*              type;
    utils::VisibilityInfo visibility;
    bool                  variability;
  };

private:
  String             name;
  QatModule*         parent;
  Vec<Member*>       members;
  Vec<StaticMember*> staticMembers;

  MemberFunction*        defaultConstructor = nullptr;
  Vec<MemberFunction*>   memberFunctions;      // Normal
  Vec<MemberFunction*>   binaryOperators;      //
  Vec<MemberFunction*>   unaryOperators;       //
  Vec<MemberFunction*>   constructors;         // Constructors
  Vec<MemberFunction*>   fromConvertors;       // From Convertors
  Vec<MemberFunction*>   toConvertors;         // To Convertors
  Vec<MemberFunction*>   staticFunctions;      // Static
  MemberFunction*        destructor = nullptr; // Destructor
  Maybe<MemberFunction*> copyConstructor;      // Copy constructor
  Maybe<MemberFunction*> moveConstructor;      // Move constructor
  Maybe<MemberFunction*> copyAssignment;       // Copy assignment operator
  Maybe<MemberFunction*> moveAssignment;       // Move assignment operator
  bool                   explicitCopy = false;
  bool                   explicitMove = false;

  utils::VisibilityInfo visibility;

  // TODO - Add support for extension functions

public:
  CoreType(QatModule* mod, String _name, Vec<Member*> _members, const utils::VisibilityInfo& _visibility,
           llvm::LLVMContext& ctx);

  useit Maybe<usize> getIndexOf(const String& member) const;
  useit bool         hasMember(const String& member) const;
  useit Member*      getMember(const String& name) const;
  useit String       getFullName() const;
  useit String       getName() const;
  useit u64          getMemberCount() const;
  useit Member*      getMemberAt(u64 index);
  useit String       getMemberNameAt(u64 index) const;
  useit QatType*     getTypeOfMember(const String& member) const;
  useit Vec<Member*>&   getMembers();
  useit bool            hasStatic(const String& name) const;
  useit bool            hasMemberFunction(const String& fnName) const;
  useit MemberFunction* getMemberFunction(const String& fnName) const;
  useit bool            hasStaticFunction(const String& fnName) const;
  useit MemberFunction* getStaticFunction(const String& fnName) const;
  useit bool            hasBinaryOperator(const String& opr, IR::QatType* type) const;
  useit MemberFunction* getBinaryOperator(const String& opr, IR::QatType* type) const;
  useit bool            hasUnaryOperator(const String& opr) const;
  useit MemberFunction* getUnaryOperator(const String& opr) const;
  useit u64             getOperatorVariantIndex(const String& opr) const;
  useit bool            hasFromConvertor(IR::QatType* type) const;
  useit MemberFunction* getFromConvertor(IR::QatType* type) const;
  useit bool            hasToConvertor(IR::QatType* type) const;
  useit MemberFunction* getToConvertor(IR::QatType* type) const;
  useit bool            hasConstructorWithTypes(Vec<IR::QatType*> types) const;
  useit MemberFunction* getConstructorWithTypes(Vec<IR::QatType*> types) const;
  useit bool            hasDefaultConstructor() const;
  useit MemberFunction* getDefaultConstructor() const;
  useit bool            hasAnyFromConvertor() const;
  useit bool            hasAnyConstructor() const;
  useit bool            hasCopyConstructor() const;
  useit MemberFunction* getCopyConstructor() const;
  useit bool            isCopyExplicit() const;
  useit bool            hasMoveConstructor() const;
  useit MemberFunction* getMoveConstructor() const;
  useit bool            hasCopyAssignment() const;
  useit MemberFunction* getCopyAssignment() const;
  useit bool            hasMoveAssignment() const;
  useit MemberFunction* getMoveAssignment() const;
  useit bool            isMoveExplicit() const;
  useit bool            isTriviallyCopyable() const;
  useit bool            hasCopy() const;
  useit bool            hasMove() const;
  useit bool            hasDestructor() const;
  useit MemberFunction* getDestructor() const;
  useit utils::VisibilityInfo getVisibility() const;
  useit QatModule*            getParent();
  useit Json                  toJson() const override;
  useit TypeKind              typeKind() const override;
  useit String                toString() const override;
  void                        addStaticMember(const String& name, QatType* type, bool variability, Value* initial,
                                              const utils::VisibilityInfo& visibility, llvm::LLVMContext& ctx);
  void                        setExplicitCopy();
  void                        setExplicitMove();
};

class TemplateCoreType : public Uniq {
private:
  String                   name;
  Vec<ast::TemplatedType*> templates;
  ast::DefineCoreType*     defineCoreType;
  QatModule*               parent;
  utils::VisibilityInfo    visibility;

  mutable Vec<TemplateVariant<CoreType>> variants;

public:
  TemplateCoreType(String name, Vec<ast::TemplatedType*> templates, ast::DefineCoreType* defineCoreType,
                   QatModule* parent, const utils::VisibilityInfo& visibInfo);

  useit String getName() const;
  useit utils::VisibilityInfo getVisibility() const;
  useit usize                 getTypeCount() const;
  useit usize                 getVariantCount() const;
  useit QatModule*            getModule() const;
  useit CoreType*             fillTemplates(Vec<QatType*>& templates, IR::Context* ctx, utils::FileRange range);
};

} // namespace qat::IR

#endif