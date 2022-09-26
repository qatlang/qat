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
    Member(QatType *_type, String _name, bool _variability,
           utils::VisibilityKind _kind, utils::FileRange _fileRange);

    QatType              *type;
    String                name;
    bool                  variability;
    utils::VisibilityKind visibilityKind;
    utils::FileRange      fileRange;

    useit nuo::Json toJson() const;
  };

  // Static member representation in the AST
  class StaticMember {
  public:
    StaticMember(QatType *_type, String _name, bool _variability,
                 Expression *_value, utils::VisibilityKind _kind,
                 utils::FileRange _fileRange);

    QatType              *type;
    String                name;
    bool                  variability;
    Expression           *value;
    utils::VisibilityKind visibilityKind;
    utils::FileRange      fileRange;

    useit nuo::Json toJson() const;
  };

private:
  String                         name;
  bool                           isPacked;
  Vec<Member *>                  members;
  Vec<StaticMember *>            staticMembers;
  Vec<MemberDefinition *>        memberDefinitions;
  Vec<ConvertorDefinition *>     convertorDefinitions;
  Vec<OperatorDefinition *>      operatorDefinitions;
  Vec<ConstructorDefinition *>   constructorDefinitions;
  mutable ConstructorDefinition *defaultConstructor   = nullptr;
  mutable DestructorDefinition  *destructorDefinition = nullptr;
  utils::VisibilityKind          visibility;

  Vec<ast::TemplatedType *>     templates;
  mutable IR::CoreType         *coreType         = nullptr;
  mutable IR::TemplateCoreType *templateCoreType = nullptr;

public:
  DefineCoreType(String _name, utils::VisibilityKind _visibility,
                 utils::FileRange          _fileRange,
                 Vec<ast::TemplatedType *> _templates, bool _isPacked = false);

  useit bool isTemplate() const;

  void       addMember(Member *mem);
  void       addStaticMember(StaticMember *stm);
  void       addMemberDefinition(MemberDefinition *mdef);
  void       addConvertorDefinition(ConvertorDefinition *cdef);
  void       addConstructorDefinition(ConstructorDefinition *cdef);
  void       addOperatorDefinition(OperatorDefinition *odef);
  void       setDestructorDefinition(DestructorDefinition *ddef);
  void       setDefaultConstructor(ConstructorDefinition *cDef);
  void       createType(IR::Context *ctx) const;
  void       defineType(IR::Context *ctx) final;
  void       define(IR::Context *ctx) final;
  useit bool hasDefaultConstructor() const;
  useit bool hasDestructor() const;
  useit IR::CoreType *getCoreType();
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::defineCoreType; }
};

} // namespace qat::ast

#endif