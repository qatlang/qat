#include "./named.hpp"

namespace qat {
namespace AST {

NamedType::NamedType(const std::string _name, const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : name(_name), QatType(_variable, _filePlacement) {}

IR::QatType *NamedType::emit(IR::Context *ctx) {
  // FIXME - Support sum types, other kinds of named types and type definitions
  // auto structType = llvm::StructType::getTypeByName(ctx->llvmContext,
  //                                                   llvm::StringRef(name));
  // if (structType == nullptr) {
  //   ctx->throw_error("Type " + name + " cannot be found",
  //   filePlacement);
  // }
  // return structType;
}

void NamedType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isConstant()) {
    file += "const ";
  }
  file += (name + " ");
}

std::string NamedType::get_name() const { return name; }

TypeKind NamedType::typeKind() { return TypeKind::Float; }

backend::JSON NamedType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "named")
      ._("name", name)
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat