#include "./global_declaration.hpp"

namespace qat {
namespace AST {

GlobalDeclaration::GlobalDeclaration(std::string _name,
                                     std::optional<QatType *> _type,
                                     Expression *_value, bool _isVariable,
                                     utils::FilePlacement _filePlacement)
    : Node(_filePlacement), name(_name), type(_type), value(_value),
      isVariable(_isVariable) {}

IR::Value *GlobalDeclaration::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void GlobalDeclaration::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isHeader) {
    if (type.has_value() ? false : (!isVariable)) {
      file += "const ";
    }
    if (type.has_value()) {
      type.value()->emitCPP(file, isHeader);
    } else {
      file += "auto ";
    }
    file += (name + " = ");
    value->emitCPP(file, isHeader);
    file += ";\n";
  }
}

nuo::Json GlobalDeclaration::toJson() const {
  return nuo::Json()
      ._("nodeType", "globalDeclaration")
      ._("name", name)
      ._("hasType", type.has_value())
      ._("type", (type.has_value() ? type.value()->toJson() : nuo::Json()))
      ._("value", value->toJson())
      ._("variability", isVariable)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat