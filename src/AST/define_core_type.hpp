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

public:
  DefineCoreType(std::string _name, bool _isPacked,
                 std::vector<CoreTypeMember> _members,
                 //  std::vector<FunctionDefinition> _memberFunctions,
                 utils::VisibilityKind visibility,
                 utils::FilePlacement _filePlacement)
      : name(_name), isPacked(_isPacked), members(_members),
        Node(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::defineCoreType; }
};
} // namespace AST
} // namespace qat

#endif