#include "./unsigned_literal.hpp"

namespace qat {
namespace AST {

UnsignedLiteral::UnsignedLiteral(std::string _value,
                                 utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

llvm::Value *UnsignedLiteral::emit(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator->llvmContext),
                                llvm::StringRef(value), 10u);
}

void UnsignedLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += value + "u";
  }
}

backend::JSON UnsignedLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "unsignedLiteral")
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat