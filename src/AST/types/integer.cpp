#include "./integer.hpp"
#include "../../IR/types/integer.hpp"
#include <string>

namespace qat {
namespace AST {

IntegerType::IntegerType(const unsigned int _bitWidth, const bool _variable,
                         const utils::FilePlacement _filePlacement)
    : bitWidth(_bitWidth), QatType(_variable, _filePlacement) {}

IR::QatType *IntegerType::emit(qat::IR::Context *ctx) {
  return new IR::IntegerType(ctx->llvmContext, bitWidth);
}

void IntegerType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  std::string value;
  file.addInclude("<cstdint>");
  if (bitWidth <= 8) {
    value = "std::int8_t";
  } else if (bitWidth <= 16) {
    value = "std::int16_t";
  } else if (bitWidth <= 32) {
    value = "std::int32_t";
  } else {
    value = "std::int64_t";
  }
  if (isConstant()) {
    file += "const ";
  }
  file += value;
  // file.addEnclosedComment("i" + std::to_string(bitWidth));
}

bool IntegerType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind IntegerType::typeKind() { return TypeKind::integer; }

backend::JSON IntegerType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "integer")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat