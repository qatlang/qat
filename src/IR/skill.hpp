#ifndef QAT_IR_SKILL_HPP
#define QAT_IR_SKILL_HPP

#include "../utils/identifier.hpp"
#include "../utils/qat_region.hpp"
#include "../utils/visibility.hpp"
#include "./generic_variant.hpp"
#include "./generics.hpp"
#include "./link_names.hpp"
#include "./types/qat_type.hpp"

#include <set>

namespace qat::ast {
class Type;
class ConvertorPrototype;
class PrerunExpression;
class DefineSkill;
class DoSkill;
} // namespace qat::ast

namespace qat::ir {

class Mod;
class Method;
class Skill;

struct TypeInSkill {
	ast::Type* astType;
	Type*      irType;

	useit static TypeInSkill get(ast::Type* astType, Type* irType) {
		return TypeInSkill{.astType = astType, .irType = irType};
	}
};

enum class SkillArgKind { NORMAL, VARIADIC };

struct SkillArg {
	TypeInSkill  type;
	SkillArgKind kind;
	Identifier   name;
	bool         isVar;

	SkillArg(SkillArgKind _kind, TypeInSkill _type, Identifier _name, bool _isVar);

	useit static SkillArg* create(TypeInSkill type, Identifier name, bool isVar) {
		return std::construct_at(OwnNormal(SkillArg), SkillArgKind::NORMAL, type, std::move(name), isVar);
	}

	useit static SkillArg* create_variadic(Identifier name) {
		return std::construct_at(OwnNormal(SkillArg), SkillArgKind::VARIADIC, TypeInSkill{nullptr, nullptr},
		                         std::move(name), false);
	}
};

enum class SkillMethodKind {
	STATIC,
	NORMAL,
	VARIATION,
	VALUED,
};

class SkillMethod {
	friend class Skill;

	Skill*          parent;
	Identifier      name;
	SkillMethodKind methodKind;
	TypeInSkill     returnType;
	Vec<SkillArg*>  arguments;

  public:
	SkillMethod(SkillMethodKind _fnTy, Skill* _parent, Identifier _name, TypeInSkill _returnType,
	            Vec<SkillArg*> _arguments);

	useit static SkillMethod* create_static_method(Skill* _parent, Identifier _name, TypeInSkill _returnType,
	                                               Vec<SkillArg*> _arguments);
	useit static SkillMethod* create_method(Skill* _parent, Identifier _name, bool _isVar, TypeInSkill _returnType,
	                                        Vec<SkillArg*> _arguments);
	useit static SkillMethod* create_valued_method(Skill* _parent, Identifier _name, TypeInSkill _returnType,
	                                               Vec<SkillArg*> _arguments);

	useit Skill* get_parent_skill() const { return parent; }

	useit SkillMethodKind get_method_kind() const { return methodKind; }

	useit Identifier get_name() const { return name; }

	useit TypeInSkill const& get_given_type() const { return returnType; }

	useit Vec<SkillArg*>& get_args() { return arguments; }

	useit usize get_arg_count() const { return arguments.size(); }

	useit SkillArg* get_arg_at(usize index) { return arguments.at(index); }

	useit String to_string() const;
};

class Skill : public Uniq {
	friend class DefinitionType;
	friend class SkillMethod;
	friend class ast::DoSkill;

	Identifier            name;
	Vec<GenericArgument*> generics;
	Mod*                  parent;
	Vec<DefinitionType*>  definitions;
	Vec<SkillMethod*>     prototypes;
	VisibilityInfo        visibInfo;

  public:
	Skill(Identifier _name, Vec<GenericArgument*> _generics, Mod* _parent, VisibilityInfo _visibInfo);

	useit static Skill* create(Identifier name, Vec<GenericArgument*> generics, Mod* parent, VisibilityInfo visibInfo) {
		return std::construct_at(OwnNormal(Skill), std::move(name), std::move(generics), parent, std::move(visibInfo));
	}

	useit String get_full_name() const;

	useit Identifier get_name() const;

	useit Mod* get_module() const;

	useit VisibilityInfo const& get_visibility() const;

	useit bool has_definition(String const& name) const;

	useit DefinitionType* get_definition(String const& name) const;

	useit bool has_any_prototype(String const& name) const;

	useit bool has_prototype(String const& name, SkillMethodKind kind) const;

	useit SkillMethod* get_prototype(String const& name, SkillMethodKind kind) const;

	LinkNames get_link_names() const;
};

class DoneSkill : public Uniq {
	friend class Method;
	friend class DefinitionType;
	friend class ast::ConvertorPrototype;

	Mod*                      parent;
	Maybe<Skill*>             skill;
	Vec<ir::GenericArgument*> generics;
	FileRange                 fileRange;
	Type*                     candidateType;
	FileRange                 typeRange;

	Vec<DefinitionType*> definitions;

	Maybe<Method*> defaultConstructor;
	Vec<Method*>   staticFunctions;
	Vec<Method*>   memberFunctions;
	Vec<Method*>   valuedMemberFunctions;
	Maybe<Method*> copyConstructor;
	Maybe<Method*> moveConstructor;
	Maybe<Method*> copyAssignment;
	Maybe<Method*> moveAssignment;
	Maybe<Method*> destructor;
	Vec<Method*>   unaryOperators;
	Vec<Method*>   normalBinaryOperators;
	Vec<Method*>   variationBinaryOperators;
	Vec<Method*>   constructors;
	Vec<Method*>   fromConvertors;
	Vec<Method*>   toConvertors;

  public:
	DoneSkill(Mod* _parentMod, Maybe<Skill*> _skill, FileRange _fileRange, Type* _candidateType, FileRange _typeRange);

	useit static DoneSkill* create_extension(Mod* parent, FileRange fileRange, Type* candidateType,
	                                         FileRange typeRange);
	useit static DoneSkill* create_normal(Mod* parent, Skill* skill, FileRange fileRange, Type* candidateType,
	                                      FileRange typeRange);

	useit bool has_definition(String const& name) const;

	useit DefinitionType* get_definition(String const& name) const;

	useit bool is_generic() const { return !generics.empty(); }
	useit bool has_generic_parameter(String const& name) {
		for (auto gen : generics) {
			if (gen->get_name().value == name) {
				return true;
			}
		}
		return false;
	}
	useit GenericArgument* get_generic_parameter(String const& name) {
		for (auto gen : generics) {
			if (gen->get_name().value == name) {
				return gen;
			}
		}
		return nullptr;
	}

	useit bool has_default_constructor() const;
	useit bool has_from_convertor(Maybe<bool> isValueVar, ir::Type* type) const;
	useit bool has_to_convertor(ir::Type* type) const;
	useit bool has_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> types) const;
	useit bool has_static_method(String const& name) const;
	useit bool has_normal_method(String const& name) const;
	useit bool has_valued_method(String const& name) const;
	useit bool has_variation_method(String const& name) const;
	useit bool has_copy_constructor() const;
	useit bool has_move_constructor() const;
	useit bool has_copy_assignment() const;
	useit bool has_move_assignment() const;
	useit bool has_destructor() const;
	useit bool has_unary_operator(String const& name) const;
	useit bool has_normal_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;
	useit bool has_variation_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;

	useit ir::Method* get_default_constructor() const;
	useit ir::Method* get_from_convertor(Maybe<bool> isValueVar, ir::Type* type) const;
	useit ir::Method* get_to_convertor(ir::Type* type) const;
	useit ir::Method* get_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> argTypes) const;
	useit ir::Method* get_static_method(String const& name) const;
	useit ir::Method* get_normal_method(String const& name) const;
	useit ir::Method* get_valued_method(String const& name) const;
	useit ir::Method* get_variation_method(String const& name) const;
	useit ir::Method* get_copy_constructor() const;
	useit ir::Method* get_move_constructor() const;
	useit ir::Method* get_copy_assignment() const;
	useit ir::Method* get_move_assignment() const;
	useit ir::Method* get_destructor() const;
	useit ir::Method* get_unary_operator(String const& name) const;
	useit ir::Method* get_normal_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;
	useit ir::Method* get_variation_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;

	useit String get_full_name() const;

	useit bool           is_type_extension() const;
	useit bool           is_normal_skill() const;
	useit Skill*         get_skill() const;
	useit FileRange      get_type_range() const;
	useit FileRange      get_file_range() const;
	useit Type*          get_ir_type() const;
	useit Mod*           get_module() const;
	useit VisibilityInfo get_visibility() const;
	useit LinkNames      get_link_names() const;
	useit String         to_string() const;
};

} // namespace qat::ir

#endif
