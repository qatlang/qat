#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

UnsignedType::UnsignedType(const unsigned int _bitWidth, const bool _variable,
                           const utils::FileRange _fileRange)
    : bitWidth(_bitWidth), QatType(_variable, _fileRange) {}

IR::QatType *UnsignedType::emit(IR::Context *ctx) {
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

nuo::Json UnsignedType::toJson() const {
  return nuo::Json()
      ._("typeKind", "unsignedInteger")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

std::string UnsignedType::toString() const {
  return (isVariable() ? "var u" : "u") + std::to_string(bitWidth);
}

} // namespace qat::ast