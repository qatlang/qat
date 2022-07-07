#include "./radix_literal.hpp"

namespace qat {
namespace AST {

RadixLiteral::RadixLiteral(std::string _value, unsigned _radix,
                           utils::FilePlacement _filePlacement)
    : value(_value), radix(_radix), Expression(_filePlacement) {}

llvm::Value *RadixLiteral::emit(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  unsigned bitWidth = 0;
  if (radix == 2) {
    bitWidth = value.length() - 2;
  } else if (radix == 8) {
    bitWidth = (value.length() - 2) * 3;
  } else if (radix == 16) {
    bitWidth = (value.length() - 2) * 4;
  } else {
    generator->throw_error("Unsupported radix", file_placement);
  }
  if (bitWidth == 0) {
    generator->throw_error("No numbers provided for radix string",
                           file_placement);
  }
  return llvm::ConstantInt::get(
      llvm::Type::getIntNTy(generator->llvmContext, bitWidth), value, radix);
}

void RadixLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    std::string val;
    if (radix == 2) {
      val = "0b" + value;
    } else if (radix == 8) {
      val = "0" + value;
    } else if (radix == 16) {
      val = "0x" + value;
    } else {
      val = value;
    }
    file += (val + " ");
  }
}

backend::JSON RadixLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "radixLiteral")
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat