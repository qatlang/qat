#include "./custom_integer_literal.hpp"
#include <nuo/json.hpp>

namespace qat {
namespace AST {

CustomIntegerLiteral::CustomIntegerLiteral(std::string _value, bool _isUnsigned,
                                           unsigned int _bitWidth,
                                           utils::FilePlacement _filePlacement)
    : value(_value), isUnsigned(_isUnsigned), bitWidth(_bitWidth),
      Expression(_filePlacement) {}

IR::Value *CustomIntegerLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("Integer literals are not assignable", file_placement);
  }
  // TODO - Implement this
}

void CustomIntegerLiteral::emitCPP(backend::cpp::File &file,
                                   bool isHeader) const {
  file.addInclude("<cstdint>");
  std::string val = "((std::";
  if (isUnsigned) {
    if (bitWidth <= 8) {
      val += "uint8_t";
    } else if (bitWidth <= 16) {
      val += "uint16_t";
    } else if (bitWidth <= 32) {
      val += "uint32_t";
    } else {
      val += "uint64_t";
    }
  } else {
    if (bitWidth <= 8) {
      val += "int8_t";
    } else if (bitWidth <= 16) {
      val += "int16_t";
    } else if (bitWidth <= 32) {
      val += "int32_t";
    } else {
      val += "int64_t";
    }
  }
  file += (val + ")" + value + ")");
}

nuo::Json CustomIntegerLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "customIntegerLiteral")
      ._("isUnsigned", isUnsigned)
      ._("bitWidth", (unsigned long long)bitWidth)
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat