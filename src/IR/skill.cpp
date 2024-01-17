#include "./skill.hpp"
#include "./qat_module.hpp"
#include "./types/reference.hpp"
#include "link_names.hpp"
#include "types/qat_type.hpp"

namespace qat::IR {

SkillArg::SkillArg(QatType* _type, Maybe<Identifier> _name, bool _isVar) : type(_type), name(_name), isVar(_isVar) {}

SkillPrototype::SkillPrototype(bool _isStatic, Skill* _skill, Identifier _name, bool _isVar,
                               IR::ReturnType* _returnType, Vec<SkillArg*> _arguments)
    : parent(_skill), isStatic(_isStatic), name(_name), isVariation(_isVar), returnType(_returnType),
      arguments(_arguments) {}

SkillPrototype* SkillPrototype::CreateStaticFn(Skill* _parent, Identifier _name, QatType* _returnType,
                                               Vec<SkillArg*> _arguments) {
  return new SkillPrototype(true, _parent, _name, false, IR::ReturnType::get(_returnType), _arguments);
}

SkillPrototype* SkillPrototype::CreateMemberFn(Skill* _parent, Identifier _name, bool _isVar, ReturnType* _returnType,
                                               Vec<SkillArg*> _arguments) {
  return new SkillPrototype(false, _parent, _name, _isVar, _returnType, _arguments);
}

Skill* SkillPrototype::getParentSkill() const { return parent; }

bool SkillPrototype::isStaticFn() const { return isStatic; }

Identifier SkillPrototype::getName() const { return name; }

bool SkillPrototype::isVariationFn() const { return isVariation; }

IR::ReturnType* SkillPrototype::getReturnType() const { return returnType; }

Vec<SkillArg*>& SkillPrototype::getArguments() { return arguments; }

usize SkillPrototype::getArgumentCount() const { return arguments.size(); }

SkillArg* SkillPrototype::getArgumentAt(usize index) { return arguments.at(index); }

String Skill::getFullName() const { return parent->getFullNameWithChild(name.value); }

Identifier Skill::getName() const { return name; }

QatModule* Skill::getParent() const { return parent; }

VisibilityInfo& Skill::getVisibility() { return visibInfo; }

bool Skill::hasPrototypeWithName(String const& name) const {
  for (auto* proto : prototypes) {
    if (proto->name.value == name) {
      return true;
    }
  }
  return false;
}

SkillPrototype* Skill::getPrototypeWithName(String const& name) const {
  for (auto* proto : prototypes) {
    if (proto->name.value == name) {
      return proto;
    }
  }
  return nullptr;
}

LinkNames Skill::getLinkNames() const {
  return parent->getLinkNames().newWith(LinkNameUnit(name.value, LinkUnitType::skill), None);
}

DoSkill::DoSkill(QatModule* _parent, Maybe<Skill*> _skill, FileRange _fileRange, QatType* _candidateType,
                 FileRange _typeRange)
    : parent(_parent), skill(_skill), fileRange(_fileRange), candidateType(_candidateType), typeRange(_typeRange) {}

DoSkill* DoSkill::CreateDefault(QatModule* parent, FileRange fileRange, QatType* candidateType, FileRange typeRange) {
  return new DoSkill(parent, None, fileRange, candidateType, typeRange);
}

DoSkill* DoSkill::CreateNormal(QatModule* parent, Skill* skill, FileRange fileRange, QatType* candidateType,
                               FileRange typeRange) {
  return new DoSkill(parent, skill, fileRange, candidateType, typeRange);
}

bool DoSkill::isDefaultForType() const { return !skill.has_value(); }

bool DoSkill::isNormalSkill() const { return skill.has_value(); }

Skill* DoSkill::getSkill() const { return skill.value_or(nullptr); }

FileRange DoSkill::getTypeRange() const { return typeRange; }

FileRange DoSkill::getFileRange() const { return fileRange; }

QatType* DoSkill::getType() const { return candidateType; }

QatModule* DoSkill::getParent() const { return parent; }

bool DoSkill::hasDefaultConstructor() const { return defaultConstructor.has_value(); }

IR::MemberFunction* DoSkill::getDefaultConstructor() const { return defaultConstructor.value(); }

bool DoSkill::hasFromConvertor(Maybe<bool> isValueVar, IR::QatType* argType) const {
  return ExpandedType::checkFromConvertor(fromConvertors, isValueVar, argType).has_value();
}

IR::MemberFunction* DoSkill::getFromConvertor(Maybe<bool> isValueVar, IR::QatType* argType) const {
  return ExpandedType::checkFromConvertor(fromConvertors, isValueVar, argType).value();
}

bool DoSkill::hasToConvertor(IR::QatType* destTy) const {
  return ExpandedType::checkToConvertor(toConvertors, destTy).has_value();
}

IR::MemberFunction* DoSkill::getToConvertor(IR::QatType* destTy) const {
  return ExpandedType::checkToConvertor(toConvertors, destTy).value();
}

bool DoSkill::hasConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const {
  return ExpandedType::checkConstructorWithTypes(constructors, argTypes).has_value();
}

IR::MemberFunction* DoSkill::getConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const {
  return ExpandedType::checkConstructorWithTypes(constructors, argTypes).value();
}

bool DoSkill::hasUnaryOperator(String const& name) const {
  return ExpandedType::checkUnaryOperator(unaryOperators, name).has_value();
}

IR::MemberFunction* DoSkill::getUnaryOperator(String const& name) const {
  return ExpandedType::checkUnaryOperator(unaryOperators, name).value();
}

bool DoSkill::hasNormalBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(normalBinaryOperators, name, argType).has_value();
}

IR::MemberFunction* DoSkill::getNormalBinaryOperator(String const&                   name,
                                                     Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(normalBinaryOperators, name, argType).value();
}

bool DoSkill::hasVariationBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(variationBinaryOperators, name, argType).has_value();
}

IR::MemberFunction* DoSkill::getVariationBinaryOperator(String const&                   name,
                                                        Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(variationBinaryOperators, name, argType).value();
}

bool DoSkill::hasStaticFunction(String const& name) const {
  return ExpandedType::checkStaticFunction(staticFunctions, name).has_value();
}

IR::MemberFunction* DoSkill::getStaticFunction(String const& name) const {
  return ExpandedType::checkStaticFunction(staticFunctions, name).value();
}

bool DoSkill::hasNormalMemberFn(String const& name) const {
  return ExpandedType::checkNormalMemberFn(memberFunctions, name).has_value();
}

IR::MemberFunction* DoSkill::getNormalMemberFn(String const& name) const {
  return ExpandedType::checkNormalMemberFn(memberFunctions, name).value();
}

bool DoSkill::hasVariationFn(String const& name) const {
  return ExpandedType::checkVariationFn(memberFunctions, name).has_value();
}

IR::MemberFunction* DoSkill::getVariationFn(String const& name) const {
  return ExpandedType::checkVariationFn(memberFunctions, name).value();
}

bool DoSkill::hasCopyConstructor() const { return copyConstructor.has_value(); }

IR::MemberFunction* DoSkill::getCopyConstructor() const { return copyConstructor.value(); }

bool DoSkill::hasCopyAssignment() const { return copyAssignment.has_value(); }

IR::MemberFunction* DoSkill::getCopyAssignment() const { return copyAssignment.value(); }

bool DoSkill::hasMoveConstructor() const { return moveConstructor.has_value(); }

IR::MemberFunction* DoSkill::getMoveConstructor() const { return moveConstructor.value(); }

bool DoSkill::hasMoveAssignment() const { return moveAssignment.has_value(); }

IR::MemberFunction* DoSkill::getMoveAssignment() const { return moveAssignment.value(); }

bool DoSkill::hasDestructor() const { return destructor.has_value(); }

IR::MemberFunction* DoSkill::getDestructor() const { return destructor.value(); }

LinkNames DoSkill::getLinkNames() const {
  return parent->getLinkNames().newWith(
      LinkNameUnit(isDefaultForType() ? "" : skill.value()->getLinkNames().toName(),
                   isDefaultForType() ? LinkUnitType::doType : LinkUnitType::doSkill,
                   {LinkNames({LinkNameUnit(candidateType->getNameForLinking(), LinkUnitType::name)}, None, nullptr)}),
      None);
}

VisibilityInfo DoSkill::getVisibility() const {
  return skill.has_value()
             ? skill.value()->getVisibility()
             : (candidateType->isExpanded() ? candidateType->asExpanded()->getVisibility() : VisibilityInfo::pub());
}

String DoSkill::toString() const {
  return "do " + (isDefaultForType() ? ("type " + candidateType->toString())
                                     : (skill.value()->getFullName() + " for " + candidateType->toString()));
}

} // namespace qat::IR