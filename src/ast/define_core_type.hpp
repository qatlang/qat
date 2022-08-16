#ifndef QAT_AST_DEFINE_CORE_HPP
#define QAT_AST_DEFINE_CORE_HPP

#include "../utils/visibility.hpp"
#include "./convertor_definition.hpp"
#include "./expression.hpp"
#include "./member_definition.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

class DefineCoreType : public Node {
public:
  class Member {
  public:
    Member(QatType *_type, String _name, bool _variability,
           const utils::VisibilityInfo &_visibility,
           utils::FileRange             _fileRange);

    QatType              *type;
    String                name;
    bool                  variability;
    utils::VisibilityInfo visibility;
    utils::FileRange      fileRange;

    useit nuo::Json toJson() const;
  };

  // Static member representation in the AST
  class StaticMember {
  public:
    StaticMember(QatType *_type, String _name, bool _variability,
                 Expression *_value, const utils::VisibilityInfo &_visibility,
                 utils::FileRange _fileRange);

    QatType              *type;
    String                name;
    bool                  variability;
    Expression           *value;
    utils::VisibilityInfo visibility;
    utils::FileRange      fileRange;

    useit nuo::Json toJson() const;
  };

private:
  String                     name;
  bool                       isPacked;
  Vec<Member *>              members;
  Vec<StaticMember *>        staticMembers;
  Vec<MemberDefinition *>    memberDefinitions;
  Vec<ConvertorDefinition *> convertorDefinitions;
  utils::VisibilityInfo      visibility;

public:
  DefineCoreType(String _name, const utils::VisibilityInfo &_visibility,
                 utils::FileRange _fileRange, bool _isPacked = false);

  void  addMember(Member *mem);
  void  addStaticMember(StaticMember *stm);
  void  addMemberDefinition(MemberDefinition *mdef);
  void  addConvertorDefinition(ConvertorDefinition *cdef);
  useit IR::Value *emit(IR::Context *ctx) override;
  useit nuo::Json toJson() const override;
  useit NodeType  nodeType() const override { return NodeType::defineCoreType; }
};

} // namespace qat::ast

#endif