#ifndef QAT_IR_SKILL_HPP
#define QAT_IR_SKILL_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./link_names.hpp"
#include "./types/qat_type.hpp"
#include "generics.hpp"

namespace qat::ast {
class Type;
class ConvertorPrototype;
} // namespace qat::ast

namespace qat::ir {

class Mod;
class Method;

class Skill;

struct SkillArg {
	ast::Type* type;
	Identifier name;
	bool       isVar;

	SkillArg(ast::Type* _type, Identifier _name, bool _isVar);
};

enum class SkillFnType {
	static_method,
	normal_method,
	variation_method,
	value_method,
};

class SkillPrototype {
	friend class Skill;
	Skill*         parent;
	Identifier     name;
	SkillFnType    fnTy;
	ast::Type*     returnType;
	Vec<SkillArg*> arguments;

  public:
	SkillPrototype(SkillFnType _fnTy, Skill* _parent, Identifier _name, ast::Type* _returnType,
	               Vec<SkillArg*> _arguments);

	useit static SkillPrototype* create_static_method(Skill* _parent, Identifier _name, ast::Type* _returnType,
	                                                  Vec<SkillArg*> _arguments);
	useit static SkillPrototype* create_method(Skill* _parent, Identifier _name, bool _isVar, ast::Type* _returnType,
	                                           Vec<SkillArg*> _arguments);
	useit static SkillPrototype* create_valued_method(Skill* _parent, Identifier _name, ast::Type* _returnType,
	                                                  Vec<SkillArg*> _arguments);

	useit Skill*      get_parent_skill() const { return parent; }
	useit SkillFnType get_method_type() const { return fnTy; }
	useit Identifier  get_name() const { return name; }
	useit ast::Type* get_return_type() const { return returnType; }
	useit Vec<SkillArg*>& get_args() { return arguments; }
	useit usize           get_arg_count() const { return arguments.size(); }
	useit SkillArg*       get_arg_at(usize index) { return arguments.at(index); }
};

class Skill : public Uniq {
	Identifier           name;
	Mod*                 parent;
	Vec<SkillPrototype*> prototypes;
	VisibilityInfo       visibInfo;

  public:
	useit String          get_full_name() const;
	useit Identifier      get_name() const;
	useit Mod*            get_module() const;
	useit VisibilityInfo& get_visibility();

	useit bool            has_proto_with_name(String const& name) const;
	useit SkillPrototype* get_proto_with_name(String const& name) const;

	LinkNames get_link_names() const;
};

class DoneSkill : public Uniq {
	friend class Method;
	friend class ast::ConvertorPrototype;

	Mod*                      parent;
	Maybe<Skill*>             skill;
	Vec<ir::GenericArgument*> generics;
	FileRange                 fileRange;
	Type*                     candidateType;
	FileRange                 typeRange;

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

	DoneSkill(Mod* _parentMod, Maybe<Skill*> _skill, FileRange _fileRange, Type* _candidateType, FileRange _typeRange);

  public:
	useit static DoneSkill* create_extension(Mod* parent, FileRange fileRange, Type* candidateType,
	                                         FileRange typeRange);
	useit static DoneSkill* create_normal(Mod* parent, Skill* skill, FileRange fileRange, Type* candidateType,
	                                      FileRange typeRange);

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
