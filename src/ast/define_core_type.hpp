#ifndef QAT_AST_DEFINE_CORE_HPP
#define QAT_AST_DEFINE_CORE_HPP

#include "../utils/visibility.hpp"
#include "./expression.hpp"
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
           utils::VisibilityInfo _visibility, utils::FileRange _fileRange);

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
  // Name of the core type
  const String name;

  // Whether the low-level structure is tightly packed
  bool isPacked;

  // Non-static fields in the core type
  Vec<Member *> members;

  // Static fields in the core type
  Vec<StaticMember *> staticMembers;

  // Visibility of the core type
  utils::VisibilityInfo visibility;

public:
  DefineCoreType(String _name, Vec<Member *> _members,
                 Vec<StaticMember *>          _staticMembers,
                 const utils::VisibilityInfo &_visibility,
                 utils::FileRange _fileRange, bool _isPacked = false);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::defineCoreType; }
};

} // namespace qat::ast

#endif