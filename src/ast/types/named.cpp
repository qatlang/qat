#include "./named.hpp"

namespace qat::ast {

NamedType::NamedType(const std::string _name, const bool _variable,
                     const utils::FileRange _fileRange)
    : name(_name), QatType(_variable, _fileRange) {}

IR::QatType *NamedType::emit(IR::Context *ctx) {
  // FIXME - Support sum types, other kinds of named types and type definitions
  // auto structType = llvm::StructType::getTypeByName(ctx->llvmContext,
  //                                                   llvm::StringRef(name));
  // if (structType == nullptr) {
  //   ctx->throw_error("Type " + name + " cannot be found",
  //   fileRange);
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

nuo::Json NamedType::toJson() const {
  return nuo::Json()
      ._("typeKind", "named")
      ._("name", name)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

std::string NamedType::toString() const {
  return (isVariable() ? "var " : "") + name;
}

} // namespace qat::ast