#ifndef QAT_IR_SKILL_HPP
#define QAT_IR_SKILL_HPP

#include "../utils/identifier.hpp"
#include "../utils/mentionable.hpp"
#include "../utils/qat_region.hpp"
#include "../utils/visibility.hpp"
#include "./generic_variant.hpp"
#include "./generics.hpp"
#include "./link_names.hpp"
#include "./types/function.hpp"
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

	static TypeInSkill get(ast::Type* astType, Type* irType) {
		return TypeInSkill{.astType = astType, .irType = irType};
	}

	String to_string() const;
};

struct SkillArg {
	TypeInSkill type;
	Identifier  name;
	bool        isVar;

	SkillArg(TypeInSkill _type, Identifier _name, bool _isVar) : type(_type), name(_name), isVar(_isVar) {}

	static SkillArg* create(TypeInSkill type, Identifier name, bool isVar) {
		return std::construct_at(OwnNormal(SkillArg), type, std::move(name), isVar);
	}
};

enum class SkillMethodKind {
	STATIC,
	NORMAL,
	VARIATION,
	VALUE,
};

struct SkillVariadics {
	VariadicsKind      kind;
	Maybe<TypeInSkill> type;

	String to_string() const {
		switch (kind) {
			case VariadicsKind::NORMAL:
				return "variadic";
			case VariadicsKind::LEGACY:
				return "variadic:legacy";
			case VariadicsKind::TYPED:
				return "variadic :: " + type.value().to_string();
		}
	}
};

class SkillMethod {
	friend class Skill;

	usize index;

	Skill*                parent;
	Identifier            name;
	SkillMethodKind       methodKind;
	TypeInSkill           returnType;
	Vec<SkillArg*>        arguments;
	Maybe<SkillVariadics> variadics;

  public:
	SkillMethod(SkillMethodKind fnTy, Skill* parent, Identifier name, TypeInSkill returnType, Vec<SkillArg*> arguments,
	            Maybe<SkillVariadics> variadics);

	static SkillMethod* create_static_method(Skill* _parent, Identifier _name, TypeInSkill _returnType,
	                                         Vec<SkillArg*> _arguments, Maybe<SkillVariadics> variadics);
	static SkillMethod* create_method(Skill* _parent, Identifier _name, bool _isVar, TypeInSkill _returnType,
	                                  Vec<SkillArg*> _arguments, Maybe<SkillVariadics> variadics);

	usize get_method_index() const { return index; }

	Skill* get_parent_skill() const { return parent; }

	SkillMethodKind get_method_kind() const { return methodKind; }

	Identifier get_name() const { return name; }

	TypeInSkill const& get_given_type() const { return returnType; }

	Vec<SkillArg*>& get_args() { return arguments; }

	usize get_arg_count() const { return arguments.size(); }

	SkillArg* get_arg_at(usize index) { return arguments.at(index); }

	bool is_variadic() const { return variadics.has_value(); }

	SkillVariadics get_variadics() const { return variadics.value(); }

	String to_string() const;
};

class Skill : public Uniq, public Mentionable {
	friend class DefinitionType;
	friend class SkillMethod;
	friend class ast::DoSkill;
	friend class DoneSkill;

	Identifier            name;
	Vec<GenericArgument*> generics;
	Mod*                  parent;
	Vec<DefinitionType*>  definitions;
	Vec<SkillMethod*>     prototypes;
	VisibilityInfo        visibInfo;
	bool                  canBePolymorph;

  public:
	Skill(Identifier _name, bool _canBePoly, Vec<GenericArgument*> _generics, Mod* _parent, VisibilityInfo _visibInfo);

	static Skill* create(Identifier name, bool canBePoly, Vec<GenericArgument*> generics, Mod* parent,
	                     VisibilityInfo visibInfo) {
		return std::construct_at(OwnNormal(Skill), std::move(name), canBePoly, std::move(generics), parent,
		                         std::move(visibInfo));
	}

	String get_full_name() const;

	Identifier get_name() const;

	bool can_be_polymorph() const { return canBePolymorph; }

	Mod* get_module() const;

	VisibilityInfo const& get_visibility() const;

	bool has_definition(String const& name) const;

	DefinitionType* get_definition(String const& name) const;

	bool has_any_prototype(String const& name) const;

	bool has_prototype(String const& name, SkillMethodKind kind) const;

	SkillMethod* get_prototype(String const& name, SkillMethodKind kind) const;

	LinkNames get_link_names() const;
};

class GenericSkill : public Uniq, public Mentionable {
	friend class ast::DefineSkill;

	Identifier                     name;
	Mod*                           parent;
	Vec<ast::GenericAbstractType*> generics;
	ast::DefineSkill*              defineSkill;
	ast::PrerunExpression*         constraint;
	VisibilityInfo                 visibInfo;

	mutable Vec<GenericVariant<Skill>> variants;
	mutable std::set<String>           variantNames;

  public:
	GenericSkill(Identifier _name, Mod* _parent, Vec<ast::GenericAbstractType*> _generics,
	             ast::DefineSkill* _defineSkill, ast::PrerunExpression* _constraint, VisibilityInfo _visibInfo);

	static GenericSkill* create(Identifier name, Mod* parent, Vec<ast::GenericAbstractType*> generics,
	                            ast::DefineSkill* defineSkill, ast::PrerunExpression* constraint,
	                            VisibilityInfo visibInfo) {
		return std::construct_at(OwnNormal(GenericSkill), std::move(name), parent, std::move(generics), defineSkill,
		                         constraint, std::move(visibInfo));
	}

	Identifier get_name() const { return name; }

	String get_full_name() const;

	usize get_type_count() const { return generics.size(); }

	bool all_types_have_defaults() const;

	usize get_variant_count() const { return variants.size(); }

	Mod* get_module() const { return parent; }

	Skill* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRangePtr range);

	ast::GenericAbstractType* get_generic_at(usize index) const { return generics.at(index); }

	VisibilityInfo const& get_visibility() const { return visibInfo; }
};

class DoneSkill : public Uniq, public Mentionable {
	friend class Method;
	friend class DefinitionType;
	friend class ast::ConvertorPrototype;

	Maybe<Identifier> name;

	Mod*                      parent;
	Maybe<Skill*>             skill;
	Vec<ir::GenericArgument*> generics;
	FileRangePtr              fileRange;
	Type*                     candidateType;
	FileRangePtr              typeRange;

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

	llvm::GlobalVariable* methodTable = nullptr;

  public:
	DoneSkill(Maybe<Identifier> _name, Mod* _parentMod, Maybe<Skill*> _skill, FileRangePtr _fileRange,
	          Type* _candidateType, FileRangePtr _typeRange);

	static DoneSkill* create_extension(Mod* parent, FileRangePtr fileRange, Type* candidateType,
	                                   FileRangePtr typeRange);
	static DoneSkill* create_normal(Maybe<Identifier> name, Mod* parent, Skill* skill, FileRangePtr fileRange,
	                                Type* candidateType, FileRangePtr typeRange);

	bool has_name() const { return name.has_value(); }

	Identifier const& get_name() const { return name.value(); }

	bool has_definition(String const& name) const;

	DefinitionType* get_definition(String const& name) const;

	bool is_generic() const { return not generics.empty(); }

	bool has_generic_parameter(String const& name) {
		for (auto gen : generics) {
			if (gen->get_name().value == name) {
				return true;
			}
		}
		return false;
	}

	GenericArgument* get_generic_parameter(String const& name) {
		for (auto gen : generics) {
			if (gen->get_name().value == name) {
				return gen;
			}
		}
		return nullptr;
	}

	llvm::GlobalVariable* get_method_table(ir::Ctx* irCtx);

	bool has_default_constructor() const;
	bool has_from_convertor(Maybe<bool> isValueVar, ir::Type* type) const;
	bool has_to_convertor(ir::Type* type) const;
	bool has_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> const& types) const;
	bool has_static_method(String const& name) const;
	bool has_normal_method(String const& name) const;
	bool has_valued_method(String const& name) const;
	bool has_variation_method(String const& name) const;
	bool has_copy_constructor() const;
	bool has_move_constructor() const;
	bool has_copy_assignment() const;
	bool has_move_assignment() const;
	bool has_destructor() const;
	bool has_unary_operator(String const& name) const;
	bool has_normal_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;
	bool has_variation_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;

	ir::Method* get_default_constructor() const;
	ir::Method* get_from_convertor(Maybe<bool> isValueVar, ir::Type* type) const;
	ir::Method* get_to_convertor(ir::Type* type) const;
	ir::Method* get_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> const& argTypes) const;
	ir::Method* get_static_method(String const& name) const;
	ir::Method* get_normal_method(String const& name) const;
	ir::Method* get_valued_method(String const& name) const;
	ir::Method* get_variation_method(String const& name) const;
	ir::Method* get_copy_constructor() const;
	ir::Method* get_move_constructor() const;
	ir::Method* get_copy_assignment() const;
	ir::Method* get_move_assignment() const;
	ir::Method* get_destructor() const;
	ir::Method* get_unary_operator(String const& name) const;
	ir::Method* get_normal_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;
	ir::Method* get_variation_binary_operator(String const& name, Pair<Maybe<bool>, ir::Type*> argType) const;

	String get_full_name() const;

	bool           is_type_extension() const;
	bool           is_normal_skill() const;
	Skill*         get_skill() const;
	FileRangePtr   get_type_range() const;
	FileRangePtr   get_file_range() const;
	Type*          get_candidate_type() const;
	Mod*           get_module() const;
	VisibilityInfo get_visibility() const;
	LinkNames      get_link_names() const;
	String         to_string() const;
};

} // namespace qat::ir

#endif
