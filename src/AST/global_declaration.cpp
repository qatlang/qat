#include "./global_declaration.hpp"

namespace qat {
namespace AST {

GlobalDeclaration::GlobalDeclaration(std::string _name,
                                     llvm::Optional<QatType *> _type,
                                     Expression *_value, bool _isVariable,
                                     utils::FilePlacement _filePlacement)
    : name(_name), type(_type), value(_value), isVariable(_isVariable),
      Node(_filePlacement) {}

IR::Value *GlobalDeclaration::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void GlobalDeclaration::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isHeader) {
    if (type.hasValue() ? false : (!isVariable)) {
      file += "const ";
    }
    if (type.hasValue()) {
      type.getValue()->emitCPP(file, isHeader);
    } else {
      file += "auto ";
    }
    file += (name + " = ");
    value->emitCPP(file, isHeader);
    file += ";\n";
  }
}

backend::JSON GlobalDeclaration::toJSON() const {
  return backend::JSON()
      ._("nodeType", "globalDeclaration")
      ._("name", name)
      ._("hasType", type.hasValue())
      ._("type",
         (type.hasValue() ? type.getValue()->toJSON() : backend::JSON()))
      ._("value", value->toJSON())
      ._("variability", isVariable)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat