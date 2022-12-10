#ifndef QAT_AST_DEFINE_CORE_HPP
#define QAT_AST_DEFINE_CORE_HPP

#include "../utils/visibility.hpp"
#include "./convertor.hpp"
#include "./expression.hpp"
#include "./member_function.hpp"
#include "./operator_function.hpp"
#include "./types/qat_type.hpp"
#include "constructor.hpp"
#include "destructor.hpp"
#include "types/templated.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

class DefineCoreType : public Node {
public:
  class Member {
  public:
    Member(QatType* _type, Identifier _name, bool _variability, utils::VisibilityKind _kind, FileRange _fileRange);

    QatType*              type;
    Identifier            name;
    bool                  variability;
    utils::VisibilityKind visibilityKind;
    FileRange             fileRange;

    useit Json toJson() const;
  };

  // Static member representation in the AST
  class StaticMember {
  public:
    StaticMember(QatType* _type, Identifier _name, bool _variability, Expression* _value, utils::VisibilityKind _kind,
                 FileRange _fileRange);

    QatType*              type;
    Identifier            name;
    bool                  variability;
    Expression*           value;
    utils::VisibilityKind visibilityKind;
    FileRange             fileRange;

    useit Json toJson() const;
  };

private:
  Identifier                     name;
  bool                           isPacked;
  Vec<Member*>                   members;
  Vec<StaticMember*>             staticMembers;
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
  utils::VisibilityKind          visibility;

  Vec<ast::TemplatedType*>      templates;
  mutable Maybe<String>         variantName;
  mutable IR::CoreType*         coreType         = nullptr;
  mutable IR::TemplateCoreType* templateCoreType = nullptr;

public:
  DefineCoreType(Identifier _name, utils::VisibilityKind _visibility, FileRange _fileRange,
                 Vec<ast::TemplatedType*> _templates, bool _isPacked = false);

  useit bool isTemplate() const;

  void       addMember(Member* mem);
  void       addStaticMember(StaticMember* stm);
  void       addMemberDefinition(MemberDefinition* mdef);
  void       addConvertorDefinition(ConvertorDefinition* cdef);
  void       addConstructorDefinition(ConstructorDefinition* cdef);
  void       addOperatorDefinition(OperatorDefinition* odef);
  void       setDestructorDefinition(DestructorDefinition* ddef);
  void       setDefaultConstructor(ConstructorDefinition* cDef);
  void       setCopyConstructor(ConstructorDefinition* cDef);
  void       setMoveConstructor(ConstructorDefinition* cDef);
  void       setCopyAssignment(OperatorDefinition* mDef);
  void       setMoveAssignment(OperatorDefinition* mDef);
  void       createType(IR::Context* ctx) const;
  void       defineType(IR::Context* ctx) final;
  void       define(IR::Context* ctx) final;
  void       setVariantName(const String& name) const;
  void       unsetVariantName() const;
  useit bool hasDefaultConstructor() const;
  useit bool hasDestructor() const;
  useit bool hasCopyConstructor() const;
  useit bool hasMoveConstructor() const;
  useit bool hasCopyAssignment() const;
  useit bool hasMoveAssignment() const;
  useit IR::CoreType* getCoreType();
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::defineCoreType; }
};

} // namespace qat::ast

#endif