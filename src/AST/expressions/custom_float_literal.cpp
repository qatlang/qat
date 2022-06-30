#include "./custom_float_literal.hpp"

namespace qat {
namespace AST {

CustomFloatLiteral::CustomFloatLiteral(std::string _value, std::string _kind,
                                       utils::FilePlacement _filePlacement)
    : value(_value), kind(_kind), Expression(_filePlacement) {}

llvm::Value *CustomFloatLiteral::generate(IR::Generator *generator) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  llvm::Type *flType = llvm::Type::getFloatTy(generator->llvmContext);
  if (kind == "f64") {
    flType = llvm::Type::getDoubleTy(generator->llvmContext);
  } else if (kind == "f80") {
    flType = llvm::Type::getX86_FP80Ty(generator->llvmContext);
  } else if (kind == "f128") {
    flType = llvm::Type::getFP128Ty(generator->llvmContext);
  } else if (kind == "fbrain") {
    flType = llvm::Type::getBFloatTy(generator->llvmContext);
  } else if (kind == "fhalf") {
    flType = llvm::Type::getHalfTy(generator->llvmContext);
  } else if (kind == "f128ppc") {
    flType = llvm::Type::getPPC_FP128Ty(generator->llvmContext);
  }
  return llvm::ConstantFP::get(llvm::Type::getFloatTy(generator->llvmContext),
                               llvm::StringRef(value) //
  );
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