#ifndef QAT_AST_MEMBER_PARENT_LIKE_HPP
#define QAT_AST_MEMBER_PARENT_LIKE_HPP

#include "destructor.hpp"
#include "member_function.hpp"
#include "operator_function.hpp"

namespace qat::ast {

class DoSkill;

class MemberParentLike {
public:
  Vec<MemberDefinition*>         memberDefinitions;
  Vec<ConvertorDefinition*>      convertorDefinitions;
  Vec<OperatorDefinition*>       operatorDefinitions;
  Vec<ConstructorDefinition*>    constructorDefinitions;
  mutable ConstructorDefinition* defaultConstructor   = nullptr;
  mutable ConstructorDefinition* copyConstructor      = nullptr;
  mutable ConstructorDefinition* moveConstructor      = nullptr;
  mutable OperatorDefinition*    copyAssignment       = nullptr;
  mutable OperatorDefinition*    moveAssignment       = nullptr;
  mutable DestructorDefinition*  destructorDefinition = nullptr;

  inline void add_method_definition(MemberDefinition* mdef) { memberDefinitions.push_back(mdef); }
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