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
    Member(QatType *_type, std::string _name, bool _variability,
           utils::VisibilityInfo _visibility, utils::FileRange _fileRange);

    QatType *type;
    std::string name;
    bool variability;
    utils::VisibilityInfo visibility;
    utils::FileRange fileRange;

    nuo::Json toJson() const;
  };

  // Static member representation in the AST
  class StaticMember {
  public:
    StaticMember(QatType *_type, std::string _name, bool _variability,
                 Expression *_value, utils::VisibilityInfo _visibility,
                 utils::FileRange _fileRange);

    QatType *type;
    std::string name;
    bool variability;
    Expression *value;
    utils::VisibilityInfo visibility;
    utils::FileRange fileRange;

    nuo::Json toJson() const;
  };

private:
  // Name of the core type
  const std::string name;

  // Whether the low-level structure is tightly packed
  bool isPacked;

  // Non-static fields in the core type
  std::vector<Member *> members;

  // Static fields in the core type
  std::vector<StaticMember *> staticMembers;

  // Visibility of the core type
  utils::VisibilityInfo visibility;

public:
  DefineCoreType(std::string _name, std::vector<Member *> _members,
                 std::vector<StaticMember *> _staticMembers,
                 utils::VisibilityInfo _visibility, utils::FileRange _fileRange,
                 bool _isPacked = false);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::defineCoreType; }
};

} // namespace qat::ast

#endif