#ifndef QAT_IR_SKILL_HPP
#define QAT_IR_SKILL_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./link_names.hpp"
#include "./types/qat_type.hpp"
#include "types/function.hpp"

namespace qat::IR {

class QatModule;
class MemberFunction;

class Skill;

struct SkillArg {
  QatType*          type;
  Maybe<Identifier> name;
  bool              isVar;

  SkillArg(QatType* _type, Maybe<Identifier> _name, bool _isVar);
};

class SkillPrototype {
  friend class Skill;
  Skill*          parent;
  bool            isStatic;
  Identifier      name;
  bool            isVariation;
  IR::ReturnType* returnType;
  Vec<SkillArg*>  arguments;

public:
  SkillPrototype(bool _isStatic, Skill* _parent, Identifier _name, bool _isVar, IR::ReturnType* _returnType,
                 Vec<SkillArg*> _arguments);

  useit static SkillPrototype* CreateStaticFn(Skill* _parent, Identifier _name, QatType* _returnType,
                                              Vec<SkillArg*> _arguments);
  useit static SkillPrototype* CreateMemberFn(Skill* _parent, Identifier _name, bool _isVar,
                                              IR::ReturnType* _returnType, Vec<SkillArg*> _arguments);

  useit Skill*     getParentSkill() const;
  useit bool       isStaticFn() const;
  useit Identifier getName() const;
  useit bool       isVariationFn() const;
  useit IR::ReturnType* getReturnType() const;
  useit Vec<SkillArg*>& getArguments();
  useit usize           getArgumentCount() const;
  useit SkillArg*       getArgumentAt(usize index);
};

class Skill {
  Identifier           name;
  QatModule*           parent;
  Vec<SkillPrototype*> prototypes;
  VisibilityInfo       visibInfo;

public:
  useit String          getFullName() const;
  useit Identifier      getName() const;
  useit QatModule*      getParent() const;
  useit VisibilityInfo& getVisibility();

  useit bool            hasPrototypeWithName(String const& name) const;
  useit SkillPrototype* getPrototypeWithName(String const& name) const;

  LinkNames getLinkNames() const;
};

class DoSkill : Uniq {
  friend class MemberFunction;
  QatModule*    parent;
  Maybe<Skill*> skill;
  FileRange     fileRange;
  QatType*      candidateType;
  FileRange     typeRange;

  Maybe<MemberFunction*> defaultConstructor;
  Vec<MemberFunction*>   staticFunctions;
  Vec<MemberFunction*>   memberFunctions;
  Maybe<MemberFunction*> copyConstructor;
  Maybe<MemberFunction*> moveConstructor;
  Maybe<MemberFunction*> copyAssignment;
  Maybe<MemberFunction*> moveAssignment;
  Maybe<MemberFunction*> destructor;
  Vec<MemberFunction*>   unaryOperators;
  Vec<MemberFunction*>   normalBinaryOperators;
  Vec<MemberFunction*>   variationBinaryOperators;
  Vec<MemberFunction*>   constructors;
  Vec<MemberFunction*>   fromConvertors;
  Vec<MemberFunction*>   toConvertors;

  DoSkill(QatModule* _parentMod, Maybe<Skill*> _skill, FileRange _fileRange, QatType* _candidateType,
          FileRange _typeRange);

public:
  useit static DoSkill* CreateDefault(QatModule* parent, FileRange fileRange, QatType* candidateType,
                                      FileRange typeRange);
  useit static DoSkill* CreateNormal(QatModule* parent, Skill* skill, FileRange fileRange, QatType* candidateType,
                                     FileRange typeRange);

  useit bool hasDefaultConstructor() const;
  useit bool hasFromConvertor(Maybe<bool> isValueVar, IR::QatType* type) const;
  useit bool hasToConvertor(IR::QatType* type) const;
  useit bool hasConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> types) const;
  useit bool hasStaticFunction(String const& name) const;
  useit bool hasNormalMemberFn(String const& name) const;
  useit bool hasVariationFn(String const& name) const;
  useit bool hasCopyConstructor() const;
  useit bool hasMoveConstructor() const;
  useit bool hasCopyAssignment() const;
  useit bool hasMoveAssignment() const;
  useit bool hasDestructor() const;
  useit bool hasUnaryOperator(String const& name) const;
  useit bool hasNormalBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const;
  useit bool hasVariationBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const;

  useit IR::MemberFunction* getDefaultConstructor() const;
  useit IR::MemberFunction* getFromConvertor(Maybe<bool> isValueVar, IR::QatType* type) const;
  useit IR::MemberFunction* getToConvertor(IR::QatType* type) const;
  useit IR::MemberFunction* getConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const;
  useit IR::MemberFunction* getStaticFunction(String const& name) const;
  useit IR::MemberFunction* getNormalMemberFn(String const& name) const;
  useit IR::MemberFunction* getVariationFn(String const& name) const;
  useit IR::MemberFunction* getCopyConstructor() const;
  useit IR::MemberFunction* getMoveConstructor() const;
  useit IR::MemberFunction* getCopyAssignment() const;
  useit IR::MemberFunction* getMoveAssignment() const;
  useit IR::MemberFunction* getDestructor() const;
  useit IR::MemberFunction* getUnaryOperator(String const& name) const;
  useit IR::MemberFunction* getNormalBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const;
  useit IR::MemberFunction* getVariationBinaryOperator(String const&                   name,
                                                       Pair<Maybe<bool>, IR::QatType*> argType) const;

  useit bool           isDefaultForType() const;
  useit bool           isNormalSkill() const;
  useit Skill*         getSkill() const;
  useit FileRange      getTypeRange() const;
  useit FileRange      getFileRange() const;
  useit QatType*       getType() const;
  useit QatModule*     getParent() const;
  useit VisibilityInfo getVisibility() const;
  useit LinkNames      getLinkNames() const;
  useit String         toString() const;
};

} // namespace qat::IR

#endif