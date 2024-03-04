#include "./skill.hpp"
#include "./qat_module.hpp"
#include "./types/reference.hpp"
#include "link_names.hpp"
#include "method.hpp"
#include "types/qat_type.hpp"

namespace qat::ir {

SkillArg::SkillArg(ast::Type* _type, Identifier _name, bool _isVar) : type(_type), name(_name), isVar(_isVar) {}

SkillPrototype::SkillPrototype(SkillFnType _fnTy, Skill* _skill, Identifier _name, ast::Type* _returnType,
                               Vec<SkillArg*> _arguments)
    : parent(_skill), name(_name), fnTy(_fnTy), returnType(_returnType), arguments(_arguments) {}

SkillPrototype* SkillPrototype::create_static_method(Skill* _parent, Identifier _name, ast::Type* _returnType,
                                                     Vec<SkillArg*> _arguments) {
  return new SkillPrototype(SkillFnType::static_method, _parent, _name, _returnType, _arguments);
}

SkillPrototype* SkillPrototype::create_method(Skill* _parent, Identifier _name, bool _isVar, ast::Type* _returnType,
                                              Vec<SkillArg*> _arguments) {
  return new SkillPrototype(_isVar ? SkillFnType::variation_method : SkillFnType::normal_method, _parent, _name,
                            _returnType, _arguments);
}

SkillPrototype* SkillPrototype::create_valued_method(Skill* _parent, Identifier _name, ast::Type* _returnType,
                                                     Vec<SkillArg*> _arguments) {
  return new SkillPrototype(SkillFnType::value_method, _parent, _name, _returnType, _arguments);
}

String Skill::get_full_name() const { return parent->get_fullname_with_child(name.value); }

Identifier Skill::get_name() const { return name; }

Mod* Skill::get_module() const { return parent; }

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

LinkNames Skill::get_link_names() const {
  return parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::skill), None);
}

DoneSkill::DoneSkill(Mod* _parent, Maybe<Skill*> _skill, FileRange _fileRange, Type* _candidateType,
                     FileRange _typeRange)
    : parent(_parent), skill(_skill), fileRange(_fileRange), candidateType(_candidateType), typeRange(_typeRange) {
  candidateType->doneSkills.push_back(this);
}

DoneSkill* DoneSkill::create_extension(Mod* parent, FileRange fileRange, Type* candidateType, FileRange typeRange) {
  return new DoneSkill(parent, None, fileRange, candidateType, typeRange);
}

DoneSkill* DoneSkill::create_normal(Mod* parent, Skill* skill, FileRange fileRange, Type* candidateType,
                                    FileRange typeRange) {
  return new DoneSkill(parent, skill, fileRange, candidateType, typeRange);
}

bool DoneSkill::is_type_extension() const { return !skill.has_value(); }

bool DoneSkill::is_normal_skill() const { return skill.has_value(); }

Skill* DoneSkill::get_skill() const { return skill.value_or(nullptr); }

FileRange DoneSkill::get_type_range() const { return typeRange; }

FileRange DoneSkill::get_file_range() const { return fileRange; }

Type* DoneSkill::get_ir_type() const { return candidateType; }

Mod* DoneSkill::get_module() const { return parent; }

bool DoneSkill::has_default_constructor() const { return defaultConstructor.has_value(); }

ir::Method* DoneSkill::get_default_constructor() const { return defaultConstructor.value(); }

bool DoneSkill::has_from_convertor(Maybe<bool> isValueVar, ir::Type* argType) const {
  return ExpandedType::check_from_convertor(fromConvertors, isValueVar, argType).has_value();
}

ir::Method* DoneSkill::get_from_convertor(Maybe<bool> isValueVar, ir::Type* argType) const {
  return ExpandedType::check_from_convertor(fromConvertors, isValueVar, argType).value();
}

bool DoneSkill::has_to_convertor(ir::Type* destTy) const {
  return ExpandedType::check_to_convertor(toConvertors, destTy).has_value();
}

ir::Method* DoneSkill::get_to_convertor(ir::Type* destTy) const {
  return ExpandedType::check_to_convertor(toConvertors, destTy).value();
}

bool DoneSkill::has_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> argTypes) const {
  return ExpandedType::check_constructor_with_types(constructors, argTypes).has_value();
}

ir::Method* DoneSkill::get_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> argTypes) const {
  return ExpandedType::check_constructor_with_types(constructors, argTypes).value();
}

bool DoneSkill::has_unary_operator(String const& name) const {
  return ExpandedType::check_unary_operator(unaryOperators, name).has_value();
}

ir::Method* DoneSkill::get_unary_operator(String const& name) const {
  return ExpandedType::check_unary_operator(unaryOperators, name).value();
}

bool DoneSkill::has_normal_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const {
  return ExpandedType::check_binary_operator(normalBinaryOperators, name, argType).has_value();
}

ir::Method* DoneSkill::get_normal_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const {
  return ExpandedType::check_binary_operator(normalBinaryOperators, name, argType).value();
}

bool DoneSkill::has_variation_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const {
  return ExpandedType::check_binary_operator(variationBinaryOperators, name, argType).has_value();
}

ir::Method* DoneSkill::get_variation_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const {
  return ExpandedType::check_binary_operator(variationBinaryOperators, name, argType).value();
}

bool DoneSkill::has_static_method(String const& name) const {
  return ExpandedType::check_static_method(staticFunctions, name).has_value();
}

ir::Method* DoneSkill::get_static_method(String const& name) const {
  return ExpandedType::check_static_method(staticFunctions, name).value();
}

bool DoneSkill::has_valued_method(String const& name) const {
  return ExpandedType::check_valued_function(valuedMemberFunctions, name).has_value();
}

ir::Method* DoneSkill::get_valued_method(String const& name) const {
  return ExpandedType::check_valued_function(valuedMemberFunctions, name).value();
}

bool DoneSkill::has_normal_method(String const& name) const {
  return ExpandedType::check_normal_method(memberFunctions, name).has_value();
}

ir::Method* DoneSkill::get_normal_method(String const& name) const {
  return ExpandedType::check_normal_method(memberFunctions, name).value();
}

bool DoneSkill::has_variation_method(String const& name) const {
  return ExpandedType::check_variation(memberFunctions, name).has_value();
}

ir::Method* DoneSkill::get_variation_method(String const& name) const {
  return ExpandedType::check_variation(memberFunctions, name).value();
}

bool DoneSkill::has_copy_constructor() const { return copyConstructor.has_value(); }

ir::Method* DoneSkill::get_copy_constructor() const { return copyConstructor.value(); }

bool DoneSkill::has_copy_assignment() const { return copyAssignment.has_value(); }

ir::Method* DoneSkill::get_copy_assignment() const { return copyAssignment.value(); }

bool DoneSkill::has_move_constructor() const { return moveConstructor.has_value(); }

ir::Method* DoneSkill::get_move_constructor() const { return moveConstructor.value(); }

bool DoneSkill::has_move_assignment() const { return moveAssignment.has_value(); }

ir::Method* DoneSkill::get_move_assignment() const { return moveAssignment.value(); }

bool DoneSkill::has_destructor() const { return destructor.has_value(); }

ir::Method* DoneSkill::get_destructor() const { return destructor.value(); }

LinkNames DoneSkill::get_link_names() const {
  return parent->get_link_names().newWith(
      LinkNameUnit(
          is_type_extension() ? "" : skill.value()->get_link_names().toName(),
          is_type_extension() ? LinkUnitType::doType : LinkUnitType::doSkill,
          {LinkNames({LinkNameUnit(candidateType->get_name_for_linking(), LinkUnitType::name)}, None, nullptr)}),
      None);
}

VisibilityInfo DoneSkill::get_visibility() const {
  return skill.has_value()
             ? skill.value()->get_visibility()
             : (candidateType->is_expanded() ? candidateType->as_expanded()->get_visibility() : VisibilityInfo::pub());
}

String DoneSkill::to_string() const {
  String genStr = ":[";
  for (usize i = 0; i < generics.size(); i++) {
    genStr += generics[i]->to_string();
    if (i != (generics.size() - 1)) {
      genStr += ", ";
    }
  }
  genStr += "]";
  return "do" + (is_generic() ? genStr : "") + " " +
         (is_type_extension() ? ("type " + candidateType->to_string())
                              : (skill.value()->get_full_name() + " for " + candidateType->to_string()));
}

} // namespace qat::ir