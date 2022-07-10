#ifndef QAT_AST_DEFINE_CORE_HPP
#define QAT_AST_DEFINE_CORE_HPP

#include "../IR/context.hpp"
#include "../utils/visibility.hpp"
#include "./expression.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat {
namespace AST {
class CoreTypeMember {
public:
  CoreTypeMember();
  QatType *type;
  std::string name;
  utils::VisibilityKind visibility;
  std::optional<Expression> value;
};

class DefineCoreType : public Node {
private:
  const std::string name;
  bool isPacked = false;
  std::vector<CoreTypeMember> members;
  utils::VisibilityInfo visibility;

public:
  DefineCoreType(std::string _name, bool isPacked,
                 std::vector<CoreTypeMember> _members,
                 utils::VisibilityKind _visibility,
                 utils::FilePlacement _filePlacement)
      : name(_name), isPacked(isPacked), members(_members), visibility(_visibility),
        Node(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::defineCoreType; }
};
} // namespace AST
} // namespace qat

#endif