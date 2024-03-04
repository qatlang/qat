#ifndef QAT_AST_MEMBER_PARENT_LIKE_HPP
#define QAT_AST_MEMBER_PARENT_LIKE_HPP

#include "../IR/member_function.hpp"
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
class DefineCoreType;

struct MethodState {
  ir::MemberParent* parent;
  ir::Method*       result;
  Maybe<bool>       defineCondition;

  MethodState(ir::MemberParent* _parent) : parent(_parent), result(nullptr), defineCondition(None) {}
  MethodState(ir::MemberParent* _parent, ir::Method* _result)
      : parent(_parent), result(_result), defineCondition(None) {}

  MethodState(ir::MemberParent* _parent, ir::Method* _result, Maybe<bool> _defineCondition)
      : parent(_parent), result(_result), defineCondition(_defineCondition) {}
};

class MethodResult {
public:
  ir::Method* fn;
  Maybe<bool> condition;

  MethodResult(ir::Method* _fn) : fn(_fn), condition(None) {}
  MethodResult(ir::Method* _fn, Maybe<bool> _cond) : fn(_fn), condition(_cond) {}
};

class MemberParentState {
public:
  Vec<MethodResult> all_methods;
  Vec<ir::Method*>  convertors;
  Vec<ir::Method*>  operators;
  Vec<ir::Method*>  constructors;
  ir::Method*       defaultConstructor;
  ir::Method*       copyConstructor;
  ir::Method*       moveConstructor;
  ir::Method*       copyAssignment;
  ir::Method*       moveAssignment;
  ir::Method*       destructor;

  MemberParentState()
      : all_methods(), convertors(), operators(), constructors(), defaultConstructor(nullptr), copyConstructor(nullptr),
        moveConstructor(nullptr), copyAssignment(nullptr), moveAssignment(nullptr), destructor(nullptr) {}

  useit static inline MemberParentState* get() { return std::construct_at(OwnNormal(MemberParentState)); }
};

class MemberParentLike {
public:
  Vec<Pair<ir::MemberParent*, MemberParentState*>> parentStates;

  MemberParentState* get_state_for(ir::MemberParent* parent) {
    for (auto& state : parentStates) {
      if (state.first->is_same(parent)) {
        return state.second;
      }
    }
    parentStates.push_back(std::make_pair(parent, MemberParentState::get()));
    return parentStates.back().second;
  }

  Vec<MethodDefinition*>      memberDefinitions;
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
      : parentStates(), memberDefinitions(), convertorDefinitions(), operatorDefinitions(), constructorDefinitions(),
        defaultConstructor(nullptr), copyConstructor(nullptr), moveConstructor(nullptr), copyAssignment(nullptr),
        moveAssignment(nullptr), destructorDefinition(nullptr) {}

  ~MemberParentLike() {
    for (auto& it : parentStates) {
      std::destroy_at(it.second);
    }
  }

  inline void add_method_definition(MethodDefinition* mdef) { memberDefinitions.push_back(mdef); }
  inline void add_convertor_definition(ConvertorDefinition* cdef) { convertorDefinitions.push_back(cdef); }
  inline void add_constructor_definition(ConstructorDefinition* cdef) { constructorDefinitions.push_back(cdef); }
  inline void add_operator_definition(OperatorDefinition* odef) { operatorDefinitions.push_back(odef); }
  inline void set_destructor_definition(DestructorDefinition* ddef) { destructorDefinition = ddef; }
  inline void set_default_constructor(ConstructorDefinition* cDef) { defaultConstructor = cDef; }
  inline void set_copy_constructor(ConstructorDefinition* cDef) { copyConstructor = cDef; }
  inline void set_move_constructor(ConstructorDefinition* cDef) { moveConstructor = cDef; }
  inline void set_copy_assignment(OperatorDefinition* mDef) { copyAssignment = mDef; }
  inline void set_move_assignment(OperatorDefinition* mDef) { moveAssignment = mDef; }

  useit inline bool has_destructor() const { return destructorDefinition != nullptr; }
  useit inline bool has_default_constructor() const { return defaultConstructor != nullptr; }
  useit inline bool has_copy_constructor() const { return copyConstructor != nullptr; }
  useit inline bool has_move_constructor() const { return moveConstructor != nullptr; }
  useit inline bool has_copy_assignment() const { return copyAssignment != nullptr; }
  useit inline bool has_move_assignment() const { return moveAssignment != nullptr; }

  useit virtual bool is_define_core_type() const { return false; }
  useit virtual bool is_done_skill() const { return false; }

  useit virtual DefineCoreType* as_define_core_type() { return nullptr; }
  useit virtual DoSkill*        as_done_skill() { return nullptr; }
};

} // namespace qat::ast

#endif