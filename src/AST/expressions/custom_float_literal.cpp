#include "./custom_float_literal.hpp"

namespace qat {
namespace AST {

CustomFloatLiteral::CustomFloatLiteral(std::string _value, std::string _kind,
                                       utils::FilePlacement _filePlacement)
    : value(_value), kind(_kind), Expression(_filePlacement) {}

IR::Value *CustomFloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("Float literals are not assignable", file_placement);
  }
  // TODO - Implement this
}

void CustomFloatLiteral::emitCPP(backend::cpp::File &file,
                                 bool isHeader) const {
  std::string val = "((";
  if (kind == "f32" || kind == "fhalf" || kind == "fbrain") {
    val += "float";
  } else if (kind == "f64") {
    val += "double";
  } else {
    val += "long double";
  }
  val += ")" + value + ")";
}

backend::JSON CustomFloatLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "customFloatLiteral")
      ._("kind", kind)
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat