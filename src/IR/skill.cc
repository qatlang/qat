#include "./skill.hpp"
#include "../ast/define_skill.hpp"
#include "../ast/expression.hpp"
#include "../ast/types/generic_abstract.hpp"
#include "./link_names.hpp"
#include "./logic.hpp"
#include "./method.hpp"
#include "./qat_module.hpp"
#include "./types/qat_type.hpp"
#include "./types/reference.hpp"

namespace qat::ir {

SkillArg::SkillArg(SkillArgKind _kind, TypeInSkill _type, Identifier _name, bool _isVar)
    : type(_type), kind(_kind), name(_name), isVar(_isVar) {}

SkillMethod::SkillMethod(SkillMethodKind _fnTy, Skill* _skill, Identifier _name, TypeInSkill _returnType,
                         Vec<SkillArg*> _arguments)
    : parent(_skill), name(_name), methodKind(_fnTy), returnType(_returnType), arguments(_arguments) {
	parent->prototypes.push_back(this);
}

SkillMethod* SkillMethod::create_static_method(Skill* _parent, Identifier _name, TypeInSkill _returnType,
                                               Vec<SkillArg*> _arguments) {
	return std::construct_at(OwnNormal(SkillMethod), SkillMethodKind::STATIC, _parent, _name, _returnType, _arguments);
}

SkillMethod* SkillMethod::create_method(Skill* _parent, Identifier _name, bool _isVar, TypeInSkill _returnType,
                                        Vec<SkillArg*> _arguments) {
	return std::construct_at(OwnNormal(SkillMethod), _isVar ? SkillMethodKind::VARIATION : SkillMethodKind::NORMAL,
	                         _parent, _name, _returnType, _arguments);
}

String SkillMethod::to_string() const {
	String result;
	switch (methodKind) {
		case SkillMethodKind::NORMAL:
			break;
		case SkillMethodKind::STATIC: {
			result += "static ";
			break;
		}
		case SkillMethodKind::VARIATION: {
			result += "var:";
			break;
		}
	}
	result += name.value;
	if (returnType.astType) {
		result += " -> " + returnType.astType->to_string() + "\n";
	}
	result += "(";
	for (usize i = 0; i < arguments.size(); i++) {
		auto* arg = arguments[i];
		if (arg->kind != SkillArgKind::VARIADIC) {
			if (arg->isVar) {
				result += "var ";
			}
			result += arg->name.value;
			result += " :: ";
			result += arg->type.astType->to_string();
		}
		if (i != (arguments.size() - 1)) {
			result += ", ";
		}
	}
	result += ")";
	return result;
}

Skill::Skill(Identifier _name, Vec<GenericArgument*> _generics, Mod* _parent, VisibilityInfo _visibInfo)
    : name(std::move(_name)), generics(std::move(_generics)), parent(_parent), visibInfo(std::move(_visibInfo)) {
	if (generics.empty()) {
		parent->skills.push_back(this);
	}
}

String Skill::get_full_name() const { return parent->get_fullname_with_child(name.value); }

Identifier Skill::get_name() const { return name; }

Mod* Skill::get_module() const { return parent; }

VisibilityInfo const& Skill::get_visibility() const { return visibInfo; }

bool Skill::has_definition(String const& name) const {
	for (auto* def : definitions) {
		if (def->get_name().value == name) {
			return true;
		}
	}
	return false;
}

DefinitionType* Skill::get_definition(String const& name) const {
	for (auto* def : definitions) {
		if (def->get_name().value == name) {
			return def;
		}
	}
	return nullptr;
}

bool Skill::has_any_prototype(String const& name) const {
	for (auto* proto : prototypes) {
		if (proto->name.value == name) {
			return true;
		}
	}
	return false;
}

bool Skill::has_prototype(String const& name, SkillMethodKind kind) const {
	for (auto* proto : prototypes) {
		if ((proto->name.value == name) && (proto->get_method_kind() == kind)) {
			return true;
		}
	}
	return false;
}

SkillMethod* Skill::get_prototype(String const& name, SkillMethodKind kind) const {
	for (auto* proto : prototypes) {
		if ((proto->name.value == name) && (proto->get_method_kind() == kind)) {
			return proto;
		}
	}
	return nullptr;
}

LinkNames Skill::get_link_names() const {
	return parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::skill), None);
}

GenericSkill::GenericSkill(Identifier _name, Mod* _parent, Vec<ast::GenericAbstractType*> _generics,
                           ast::DefineSkill* _defineSkill, ast::PrerunExpression* _constraint,
                           VisibilityInfo _visibInfo)
    : name(std::move(_name)), parent(_parent), generics(std::move(_generics)), defineSkill(_defineSkill),
      constraint(_constraint), visibInfo(std::move(_visibInfo)) {
	parent->genericSkills.push_back(this);
}

bool GenericSkill::all_types_have_defaults() const {
	for (auto gen : generics) {
		if (not gen->hasDefault()) {
			return false;
		}
	}
	return true;
}

Skill* GenericSkill::fill_generics(Vec<ir::GenericToFill*>& toFillTypes, ir::Ctx* irCtx, FileRange range) {
	for (auto& var : variants) {
		SHOW("Core type variant: " << var.get()->get_full_name())
		if (var.check(irCtx, [&](const String& msg, const FileRange& rng) { irCtx->Error(msg, rng); }, toFillTypes)) {
			return var.get();
		}
	}
	ir::fill_generics(irCtx, generics, toFillTypes, range);
	if (constraint != nullptr) {
		auto checkVal = constraint->emit(ast::EmitCtx::get(irCtx, parent));
		if (not checkVal->get_ir_type()->is_bool()) {
			irCtx->Error("The constraints for generic parameters should be of " + irCtx->color("bool") +
			                 " type. Got an expression of " + irCtx->color(checkVal->get_ir_type()->to_string()),
			             constraint->fileRange);
		}
		if (not llvm::cast<llvm::ConstantInt>(checkVal->get_llvm_constant())->getValue().getBoolValue()) {
			irCtx->Error("The provided parameters for the generic skill do not satisfy the constraints", range,
			             Pair<String, FileRange>{"The constraint can be found here", constraint->fileRange});
		}
	}
	Vec<ir::GenericArgument*> genParams;
	for (auto genAb : generics) {
		genParams.push_back(genAb->toIRGenericType());
	}
	auto variantName = ir::Logic::get_generic_variant_name(name.value, toFillTypes);
	if (variantNames.contains(variantName)) {
		irCtx->Error("Repeating variant name: " + variantName, range);
	}
	variantNames.insert(variantName);
	irCtx->add_active_generic(
	    ir::GenericEntityMarker{
	        variantName,
	        ir::GenericEntityType::skill,
	        range,
	        0u,
	        genParams,
	    },
	    true);
	ir::Skill* skill = defineSkill->create_skill(toFillTypes, parent, irCtx);
	defineSkill->create_type_definitions(skill, parent, irCtx);
	defineSkill->create_methods(skill, parent, irCtx);
	for (auto* gen : generics) {
		gen->unset();
	}
	if (irCtx->get_active_generic().warningCount > 0) {
		auto count = irCtx->get_active_generic().warningCount;
		irCtx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
		                   " generated while creating generic variant " + irCtx->highlightWarning(variantName),
		               range);
		irCtx->remove_active_generic();
	} else {
		irCtx->remove_active_generic();
	}
	return skill;
}

DoneSkill::DoneSkill(Mod* _parent, Maybe<Skill*> _skill, FileRange _fileRange, Type* _candidateType,
                     FileRange _typeRange)
    : parent(_parent), skill(_skill), fileRange(_fileRange), candidateType(_candidateType), typeRange(_typeRange) {
	candidateType->doneSkills.push_back(this);
}

DoneSkill* DoneSkill::create_extension(Mod* parent, FileRange fileRange, Type* candidateType, FileRange typeRange) {
	return std::construct_at(OwnNormal(DoneSkill), parent, None, fileRange, candidateType, typeRange);
}

DoneSkill* DoneSkill::create_normal(Mod* parent, Skill* skill, FileRange fileRange, Type* candidateType,
                                    FileRange typeRange) {
	return std::construct_at(OwnNormal(DoneSkill), parent, skill, fileRange, candidateType, typeRange);
}

bool DoneSkill::has_definition(String const& name) const {
	for (auto* def : definitions) {
		if (def->get_name().value == name) {
			return true;
		}
	}
	return false;
}

DefinitionType* DoneSkill::get_definition(String const& name) const {
	for (auto* def : definitions) {
		if (def->get_name().value == name) {
			return def;
		}
	}
	return nullptr;
}

String DoneSkill::get_full_name() const {
	return (skill.has_value() ? (skill.value()->get_full_name() + ":") : "") + "do:[" + candidateType->to_string() +
	       "]";
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
	return skill.has_value() ? skill.value()->get_visibility()
	                         : (candidateType->is_expanded() ? candidateType->as_expanded()->get_visibility()
	                                                         : VisibilityInfo::pub());
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
