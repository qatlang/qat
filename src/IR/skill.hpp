#ifndef QAT_IR_SKILL_HPP
#define QAT_IR_SKILL_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./link_names.hpp"
#include "./types/qat_type.hpp"
#include "types/function.hpp"

namespace qat::ast {
class QatType;
}

namespace qat::IR {

class QatModule;
class MemberFunction;

class Skill;

struct SkillArg {
  ast::QatType* type;
  Identifier    name;
  bool          isVar;

  SkillArg(ast::QatType* _type, Identifier _name, bool _isVar);
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
  ast::QatType*  returnType;
  Vec<SkillArg*> arguments;

public:
  SkillPrototype(SkillFnType _fnTy, Skill* _parent, Identifier _name, ast::QatType* _returnType,
                 Vec<SkillArg*> _arguments);

  useit static SkillPrototype* create_static_method(Skill* _parent, Identifier _name, ast::QatType* _returnType,
                                                    Vec<SkillArg*> _arguments);
  useit static SkillPrototype* create_method(Skill* _parent, Identifier _name, bool _isVar, ast::QatType* _returnType,
                                             Vec<SkillArg*> _arguments);
  useit static SkillPrototype* create_valued_method(Skill* _parent, Identifier _name, ast::QatType* _returnType,
                                                    Vec<SkillArg*> _arguments);

  useit inline Skill*          get_parent_skill() const { return parent; }
  useit inline SkillFnType     get_method_type() const { return fnTy; }
  useit inline Identifier      get_name() const { return name; }
  useit inline ast::QatType*   get_return_type() const { return returnType; }
  useit inline Vec<SkillArg*>& get_args() { return arguments; }
  useit inline usize           get_arg_count() const { return arguments.size(); }
  useit inline SkillArg*       get_arg_at(usize index) { return arguments.at(index); }
};

class Skill : public Uniq {
  Identifier           name;
  QatModule*           parent;
  Vec<SkillPrototype*> prototypes;
  VisibilityInfo       visibInfo;

public:
  useit String          get_full_name() const;
  useit Identifier      get_name() const;
  useit QatModule*      get_module() const;
  useit VisibilityInfo& get_visibility();

  useit bool            has_proto_with_name(String const& name) const;
  useit SkillPrototype* get_proto_with_name(String const& name) const;

  LinkNames getLinkNames() const;
};

class DoneSkill : public Uniq {
  friend class MemberFunction;
  QatModule*    parent;
  Maybe<Skill*> skill;
  FileRange     fileRange;
  QatType*      candidateType;
  FileRange     typeRange;

  Maybe<MemberFunction*> defaultConstructor;
  Vec<MemberFunction*>   staticFunctions;
  Vec<MemberFunction*>   memberFunctions;
  Vec<MemberFunction*>   valuedMemberFunctions;
  Maybe<MemberFunction*> copyConstructor;
  Maybe<MemberFunction*> moveConstructor;
  Maybe<MemberFunction*> copyAssignment;
  Maybe<MemberFunction*> moveAssignment;
  Maybe<MemberFunction*> destructor;
  Vec<MemberFunction*>   unaryOperators;
  Vec<MemberFunction*>   normalBinaryOperators;
  Vec<MemberFunction*>   variationBinaryOperators;
  Vec<MemberFunction*>   constructors;
  Vec<MemberFunction*>   fromConvertors;
  Vec<MemberFunction*>   toConvertors;

  DoneSkill(QatModule* _parentMod, Maybe<Skill*> _skill, FileRange _fileRange, QatType* _candidateType,
            FileRange _typeRange);

public:
  useit static DoneSkill* create_default(QatModule* parent, FileRange fileRange, QatType* candidateType,
                                         FileRange typeRange);
  useit static DoneSkill* CreateNormal(QatModule* parent, Skill* skill, FileRange fileRange, QatType* candidateType,
                                       FileRange typeRange);

  useit bool hasDefaultConstructor() const;
  useit bool hasFromConvertor(Maybe<bool> isValueVar, IR::QatType* type) const;
  useit bool hasToConvertor(IR::QatType* type) const;
  useit bool hasConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> types) const;
  useit bool hasStaticFunction(String const& name) const;
  useit bool hasNormalMemberFn(String const& name) const;
  useit bool has_valued_function(String const& name) const;
  useit bool hasVariationFn(String const& name) const;
  useit bool hasCopyConstructor() const;
  useit bool hasMoveConstructor() const;
  useit bool hasCopyAssignment() const;
  useit bool hasMoveAssignment() const;
  useit bool hasDestructor() const;
  useit bool hasUnaryOperator(String const& name) const;
  useit bool hasNormalBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const;
  useit bool hasVariationBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const;

  useit IR::MemberFunction* getDefaultConstructor() const;
  useit IR::MemberFunction* getFromConvertor(Maybe<bool> isValueVar, IR::QatType* type) const;
  useit IR::MemberFunction* getToConvertor(IR::QatType* type) const;
  useit IR::MemberFunction* getConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const;
  useit IR::MemberFunction* getStaticFunction(String const& name) const;
  useit IR::MemberFunction* getNormalMemberFn(String const& name) const;
  useit IR::MemberFunction* get_valued_function(String const& name) const;
  useit IR::MemberFunction* getVariationFn(String const& name) const;
  useit IR::MemberFunction* getCopyConstructor() const;
  useit IR::MemberFunction* getMoveConstructor() const;
  useit IR::MemberFunction* getCopyAssignment() const;
  useit IR::MemberFunction* getMoveAssignment() const;
  useit IR::MemberFunction* getDestructor() const;
  useit IR::MemberFunction* getUnaryOperator(String const& name) const;
  useit IR::MemberFunction* getNormalBinaryOperator(String const& name, Pair<Maybe<bool>, IR::QatType*> argType) const;
  useit IR::MemberFunction* getVariationBinaryOperator(String const&                   name,
                                                       Pair<Maybe<bool>, IR::QatType*> argType) const;

  useit bool           isDefaultForType() const;
  useit bool           isNormalSkill() const;
  useit Skill*         getSkill() const;
  useit FileRange      getTypeRange() const;
  useit FileRange      getFileRange() const;
  useit QatType*       getType() const;
  useit QatModule*     getParent() const;
  useit VisibilityInfo getVisibility() const;
  useit LinkNames      getLinkNames() const;
  useit String         toString() const;
};

} // namespace qat::IR

#endif