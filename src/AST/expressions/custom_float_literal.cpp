#include "./custom_float_literal.hpp"

namespace qat {
namespace AST {

CustomFloatLiteral::CustomFloatLiteral(std::string _value, std::string _kind,
                                       utils::FilePlacement _filePlacement)
    : value(_value), kind(_kind), Expression(_filePlacement) {}

llvm::Value *CustomFloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("This expression is not assignable", file_placement);
  }
  llvm::Type *flType = llvm::Type::getFloatTy(ctx->llvmContext);
  if (kind == "f64") {
    flType = llvm::Type::getDoubleTy(ctx->llvmContext);
  } else if (kind == "f80") {
    flType = llvm::Type::getX86_FP80Ty(ctx->llvmContext);
  } else if (kind == "f128") {
    flType = llvm::Type::getFP128Ty(ctx->llvmContext);
  } else if (kind == "fbrain") {
    flType = llvm::Type::getBFloatTy(ctx->llvmContext);
  } else if (kind == "fhalf") {
    flType = llvm::Type::getHalfTy(ctx->llvmContext);
  } else if (kind == "f128ppc") {
    flType = llvm::Type::getPPC_FP128Ty(ctx->llvmContext);
  }
  return llvm::ConstantFP::get(flType, llvm::StringRef(value));
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