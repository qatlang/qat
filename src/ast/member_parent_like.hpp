#ifndef QAT_AST_MEMBER_PARENT_LIKE_HPP
#define QAT_AST_MEMBER_PARENT_LIKE_HPP

#include "../IR/method.hpp"
#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "../utils/qat_region.hpp"

namespace qat::ast {

class DoSkill;
class OperatorDefinition;
class DestructorDefinition;
class ConvertorDefinition;
class MethodDefinition;
class ConstructorDefinition;
class DefineStructType;
class TypeDefinition;

struct MethodState {
	ir::MethodParent*   parent;
	ir::Method*         result;
	Maybe<bool>         defineCondition;
	Maybe<ir::MetaInfo> metaInfo;

	MethodState(ir::MethodParent* _parent) : parent(_parent), result(nullptr), defineCondition(None) {}

	MethodState(ir::MethodParent* _parent, ir::Method* _result)
	    : parent(_parent), result(_result), defineCondition(None) {}

	MethodState(ir::MethodParent* _parent, ir::Method* _result, Maybe<bool> _defineCondition,
	            Maybe<ir::MetaInfo> _metaInfo)
	    : parent(_parent), result(_result), defineCondition(_defineCondition), metaInfo(std::move(_metaInfo)) {}
};

struct TypeInParentState {
	bool  isParentSkill;
	void* parent;

	ir::DefinitionType* result;
	Maybe<bool>         defineCondition;
	Maybe<ir::MetaInfo> metaInfo;
};

struct MethodParentState {
	Vec<TypeInParentState> definitions;

	Vec<MethodState> allMethods;
	Vec<MethodState> convertors;
	Vec<MethodState> operators;
	Vec<MethodState> constructors;
	MethodState      defaultConstructor;
	MethodState      copyConstructor;
	MethodState      moveConstructor;
	MethodState      copyAssignment;
	MethodState      moveAssignment;
	MethodState      destructor;

	MethodParentState(ir::MethodParent* parent)
	    : allMethods(), convertors(), operators(), constructors(), defaultConstructor(parent), copyConstructor(parent),
	      moveConstructor(parent), copyAssignment(parent), moveAssignment(parent), destructor(parent) {}

	useit static MethodParentState* get(ir::MethodParent* parent) {
		return std::construct_at(OwnNormal(MethodParentState), parent);
	}
};

class MemberParentLike {
  public:
	Vec<Pair<ir::MethodParent*, MethodParentState*>> parentStates;

	MethodParentState* get_state_for(ir::MethodParent* parent) {
		for (auto& state : parentStates) {
			if (state.first->is_same(parent)) {
				return state.second;
			}
		}
		parentStates.push_back(std::make_pair(parent, MethodParentState::get(parent)));
		return parentStates.back().second;
	}

	Vec<TypeDefinition*>        typeDefinitions;
	Vec<MethodDefinition*>      methodDefinitions;
	Vec<ConvertorDefinition*>   convertorDefinitions;
	Vec<OperatorDefinition*>    operatorDefinitions;
	Vec<ConstructorDefinition*> constructorDefinitions;
	ConstructorDefinition*      defaultConstructor   = nullptr;
	ConstructorDefinition*      copyConstructor      = nullptr;
	ConstructorDefinition*      moveConstructor      = nullptr;
	OperatorDefinition*         copyAssignment       = nullptr;
	OperatorDefinition*         moveAssignment       = nullptr;
	DestructorDefinition*       destructorDefinition = nullptr;

	MemberParentLike()
	    : parentStates(), methodDefinitions(), convertorDefinitions(), operatorDefinitions(), constructorDefinitions(),
	      defaultConstructor(nullptr), copyConstructor(nullptr), moveConstructor(nullptr), copyAssignment(nullptr),
	      moveAssignment(nullptr), destructorDefinition(nullptr) {}

	~MemberParentLike() {
		for (auto& it : parentStates) {
			std::destroy_at(it.second);
		}
	}

	void add_type_definition(TypeDefinition* tyDef) { typeDefinitions.push_back(tyDef); }

	void add_method_definition(MethodDefinition* mdef) { methodDefinitions.push_back(mdef); }

	void add_convertor_definition(ConvertorDefinition* cdef) { convertorDefinitions.push_back(cdef); }

	void add_constructor_definition(ConstructorDefinition* cdef) { constructorDefinitions.push_back(cdef); }

	void add_operator_definition(OperatorDefinition* odef) { operatorDefinitions.push_back(odef); }

	void set_destructor_definition(DestructorDefinition* ddef) { destructorDefinition = ddef; }

	void set_default_constructor(ConstructorDefinition* cDef) { defaultConstructor = cDef; }

	void set_copy_constructor(ConstructorDefinition* cDef) { copyConstructor = cDef; }

	void set_move_constructor(ConstructorDefinition* cDef) { moveConstructor = cDef; }

	void set_copy_assignment(OperatorDefinition* mDef) { copyAssignment = mDef; }

	void set_move_assignment(OperatorDefinition* mDef) { moveAssignment = mDef; }

	useit bool has_destructor() const { return destructorDefinition != nullptr; }

	useit bool has_default_constructor() const { return defaultConstructor != nullptr; }

	useit bool has_copy_constructor() const { return copyConstructor != nullptr; }

	useit bool has_move_constructor() const { return moveConstructor != nullptr; }

	useit bool has_copy_assignment() const { return copyAssignment != nullptr; }

	useit bool has_move_assignment() const { return moveAssignment != nullptr; }

	useit virtual bool is_define_struct_type() const { return false; }

	useit virtual bool is_done_skill() const { return false; }

	useit virtual DefineStructType* as_define_struct_type() { return nullptr; }

	useit virtual DoSkill* as_done_skill() { return nullptr; }
};

} // namespace qat::ast

#endif
