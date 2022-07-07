#include "./size_of_type.hpp"

namespace qat {
namespace AST {

SizeOfType::SizeOfType(QatType *_type, utils::FilePlacement _filePlacement)
    : type(_type), Expression(_filePlacement) {}

llvm::Value *SizeOfType::emit(IR::Generator *generator) {
  return llvm::ConstantExpr::getSizeOf(type->emit(generator));
}

void SizeOfType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "sizeof(";
    type->emitCPP(file, isHeader);
    file += ")";
  }
}

backend::JSON SizeOfType::toJSON() const {
  return backend::JSON()
      ._("nodeType", "sizeOfType")
      ._("type", type->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat