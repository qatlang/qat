#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat {
namespace AST {

UnsignedType::UnsignedType(const unsigned int _bitWidth, const bool _variable,
                           const utils::FilePlacement _filePlacement)
    : bitWidth(_bitWidth), QatType(_variable, _filePlacement) {}

IR::QatType *UnsignedType::emit(qat::IR::Context *ctx) {
  return new IR::UnsignedType(bitWidth);
}

void UnsignedType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  std::string value;
  file.addInclude("<cstdint>");
  if (bitWidth <= 8) {
    value = "std::uint8_t";
  } else if (bitWidth <= 16) {
    value = "std::uint16_t";
  } else if (bitWidth <= 32) {
    value = "std::uint32_t";
  } else {
    value = "std::uint64_t";
  }
  if (isConstant()) {
    file += "const ";
  }
  file += value;
  // file.addEnclosedComment("u" + std::to_string(bitWidth));
}

bool UnsignedType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind UnsignedType::typeKind() { return TypeKind::unsignedInteger; }

backend::JSON UnsignedType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "unsignedInteger")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

std::string UnsignedType::toString() const {
  return (isVariable() ? "var u" : "u") + std::to_string(bitWidth);
}

} // namespace AST
} // namespace qat