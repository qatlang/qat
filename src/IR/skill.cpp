#include "./skill.hpp"
#include "./qat_module.hpp"
#include "./types/reference.hpp"
#include "link_names.hpp"
#include "member_function.hpp"
#include "types/qat_type.hpp"

namespace qat::IR {

SkillArg::SkillArg(ast::QatType* _type, Identifier _name, bool _isVar) : type(_type), name(_name), isVar(_isVar) {}

SkillPrototype::SkillPrototype(SkillFnType _fnTy, Skill* _skill, Identifier _name, ast::QatType* _returnType,
                               Vec<SkillArg*> _arguments)
    : parent(_skill), name(_name), fnTy(_fnTy), returnType(_returnType), arguments(_arguments) {}

SkillPrototype* SkillPrototype::create_static_method(Skill* _parent, Identifier _name, ast::QatType* _returnType,
                                                     Vec<SkillArg*> _arguments) {
  return new SkillPrototype(SkillFnType::static_method, _parent, _name, _returnType, _arguments);
}

SkillPrototype* SkillPrototype::create_method(Skill* _parent, Identifier _name, bool _isVar, ast::QatType* _returnType,
                                              Vec<SkillArg*> _arguments) {
  return new SkillPrototype(_isVar ? SkillFnType::variation_method : SkillFnType::normal_method, _parent, _name,
                            _returnType, _arguments);
}

SkillPrototype* SkillPrototype::create_valued_method(Skill* _parent, Identifier _name, ast::QatType* _returnType,
                                                     Vec<SkillArg*> _arguments) {
  return new SkillPrototype(SkillFnType::value_method, _parent, _name, _returnType, _arguments);
}

String Skill::get_full_name() const { return parent->getFullNameWithChild(name.value); }

Identifier Skill::get_name() const { return name; }

QatModule* Skill::get_module() const { return parent; }

VisibilityInfo& Skill::get_visibility() { return visibInfo; }

bool Skill::has_proto_with_name(String const& name) const {
  for (auto* proto : prototypes) {
    if (proto->name.value == name) {
      return true;
    }
  }
  return false;
}

SkillPrototype* Skill::get_proto_with_name(String const& name) const {
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

DoneSkill::DoneSkill(QatModule* _parent, Maybe<Skill*> _skill, FileRange _fileRange, QatType* _candidateType,
                     FileRange _typeRange)
    : parent(_parent), skill(_skill), fileRange(_fileRange), candidateType(_candidateType), typeRange(_typeRange) {
  candidateType->doneSkills.push_back(this);
}

DoneSkill* DoneSkill::create_default(QatModule* parent, FileRange fileRange, QatType* candidateType,
                                     FileRange typeRange) {
  return new DoneSkill(parent, None, fileRange, candidateType, typeRange);
}

DoneSkill* DoneSkill::CreateNormal(QatModule* parent, Skill* skill, FileRange fileRange, QatType* candidateType,
                                   FileRange typeRange) {
  return new DoneSkill(parent, skill, fileRange, candidateType, typeRange);
}

bool DoneSkill::isDefaultForType() const { return !skill.has_value(); }

bool DoneSkill::isNormalSkill() const { return skill.has_value(); }

Skill* DoneSkill::getSkill() const { return skill.value_or(nullptr); }

FileRange DoneSkill::getTypeRange() const { return typeRange; }

FileRange DoneSkill::getFileRange() const { return fileRange; }

QatType* DoneSkill::getType() const { return candidateType; }

QatModule* DoneSkill::getParent() const { return parent; }

bool DoneSkill::hasDefaultConstructor() const { return defaultConstructor.has_value(); }

IR::MemberFunction* DoneSkill::getDefaultConstructor() const { return defaultConstructor.value(); }

bool DoneSkill::hasFromConvertor(Maybe<bool> isValueVar, IR::QatType* argType) const {
  return ExpandedType::checkFromConvertor(fromConvertors, isValueVar, argType).has_value();
}

IR::MemberFunction* DoneSkill::getFromConvertor(Maybe<bool> isValueVar, IR::QatType* argType) const {
  return ExpandedType::checkFromConvertor(fromConvertors, isValueVar, argType).value();
}

bool DoneSkill::hasToConvertor(IR::QatType* destTy) const {
  return ExpandedType::checkToConvertor(toConvertors, destTy).has_value();
}

IR::MemberFunction* DoneSkill::getToConvertor(IR::QatType* destTy) const {
  return ExpandedType::checkToConvertor(toConvertors, destTy).value();
}

bool DoneSkill::hasConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const {
  return ExpandedType::checkConstructorWithTypes(constructors, argTypes).has_value();
}

IR::MemberFunction* DoneSkill::getConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const {
  return ExpandedType::checkConstructorWithTypes(constructors, argTypes).value();
}

bool DoneSkill::hasUnaryOperator(String const& name) const {
  return ExpandedType::checkUnaryOperator(unaryOperators, name).has_value();
}

IR::MemberFunction* DoneSkill::getUnaryOperator(String const& name) const {
  return ExpandedType::checkUnaryOperator(unaryOperators, name).value();
}

bool DoneSkill::hasNormalBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(normalBinaryOperators, name, argType).has_value();
}

IR::MemberFunction* DoneSkill::getNormalBinaryOperator(String const&                   name,
                                                       Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(normalBinaryOperators, name, argType).value();
}

bool DoneSkill::hasVariationBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(variationBinaryOperators, name, argType).has_value();
}

IR::MemberFunction* DoneSkill::getVariationBinaryOperator(String const&                   name,
                                                          Pair<Maybe<bool>, IR::QatType*> argType) const {
  return ExpandedType::checkBinaryOperator(variationBinaryOperators, name, argType).value();
}

bool DoneSkill::hasStaticFunction(String const& name) const {
  return ExpandedType::checkStaticFunction(staticFunctions, name).has_value();
}

IR::MemberFunction* DoneSkill::getStaticFunction(String const& name) const {
  return ExpandedType::checkStaticFunction(staticFunctions, name).value();
}

bool DoneSkill::has_valued_function(String const& name) const {
  return ExpandedType::check_valued_function(valuedMemberFunctions, name).has_value();
}

IR::MemberFunction* DoneSkill::get_valued_function(String const& name) const {
  return ExpandedType::check_valued_function(valuedMemberFunctions, name).value();
}

bool DoneSkill::hasNormalMemberFn(String const& name) const {
  return ExpandedType::checkNormalMemberFn(memberFunctions, name).has_value();
}

IR::MemberFunction* DoneSkill::getNormalMemberFn(String const& name) const {
  return ExpandedType::checkNormalMemberFn(memberFunctions, name).value();
}

bool DoneSkill::hasVariationFn(String const& name) const {
  return ExpandedType::checkVariationFn(memberFunctions, name).has_value();
}

IR::MemberFunction* DoneSkill::getVariationFn(String const& name) const {
  return ExpandedType::checkVariationFn(memberFunctions, name).value();
}

bool DoneSkill::hasCopyConstructor() const { return copyConstructor.has_value(); }

IR::MemberFunction* DoneSkill::getCopyConstructor() const { return copyConstructor.value(); }

bool DoneSkill::hasCopyAssignment() const { return copyAssignment.has_value(); }

IR::MemberFunction* DoneSkill::getCopyAssignment() const { return copyAssignment.value(); }

bool DoneSkill::hasMoveConstructor() const { return moveConstructor.has_value(); }

IR::MemberFunction* DoneSkill::getMoveConstructor() const { return moveConstructor.value(); }

bool DoneSkill::hasMoveAssignment() const { return moveAssignment.has_value(); }

IR::MemberFunction* DoneSkill::getMoveAssignment() const { return moveAssignment.value(); }

bool DoneSkill::hasDestructor() const { return destructor.has_value(); }

IR::MemberFunction* DoneSkill::getDestructor() const { return destructor.value(); }

LinkNames DoneSkill::getLinkNames() const {
  return parent->getLinkNames().newWith(
      LinkNameUnit(isDefaultForType() ? "" : skill.value()->getLinkNames().toName(),
                   isDefaultForType() ? LinkUnitType::doType : LinkUnitType::doSkill,
                   {LinkNames({LinkNameUnit(candidateType->getNameForLinking(), LinkUnitType::name)}, None, nullptr)}),
      None);
}

VisibilityInfo DoneSkill::getVisibility() const {
  return skill.has_value()
             ? skill.value()->get_visibility()
             : (candidateType->isExpanded() ? candidateType->asExpanded()->getVisibility() : VisibilityInfo::pub());
}

String DoneSkill::toString() const {
  return "do " + (isDefaultForType() ? ("type " + candidateType->toString())
                                     : (skill.value()->get_full_name() + " for " + candidateType->toString()));
}

} // namespace qat::IR