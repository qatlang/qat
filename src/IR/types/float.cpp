#include "./float.hpp"

namespace qat::IR {

FloatType::FloatType(const FloatTypeKind _kind) : kind(_kind) {}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

llvm::Type *FloatType::emitLLVM(llvmHelper &help) const {
  switch (kind) {
  case FloatTypeKind::_brain: {
    return llvm::Type::getBFloatTy(help.llctx);
  }
  case FloatTypeKind::_half: {
    return llvm::Type::getHalfTy(help.llctx);
  }
  case FloatTypeKind::_32: {
    return llvm::Type::getFloatTy(help.llctx);
  }
  case FloatTypeKind::_64: {
    return llvm::Type::getDoubleTy(help.llctx);
  }
  case FloatTypeKind::_80: {
    return llvm::Type::getX86_FP80Ty(help.llctx);
  }
  case FloatTypeKind::_128PPC: {
    return llvm::Type::getPPC_FP128Ty(help.llctx);
  }
  case FloatTypeKind::_128: {
    return llvm::Type::getFP128Ty(help.llctx);
  }
  }
}

void FloatType::emitCPP(cpp::File &file) const {
  switch (kind) {
  case FloatTypeKind::_brain:
  case FloatTypeKind::_half:
  case FloatTypeKind::_32: {
    file << "float ";
    break;
  }
  default: {
    file << "double ";
  }
  }
}

String FloatType::toString() const {
  switch (kind) {
  case FloatTypeKind::_half: {
    return "fhalf";
  }
  case FloatTypeKind::_brain: {
    return "fbrain";
  }
  case FloatTypeKind::_32: {
    return "f32";
  }
  case FloatTypeKind::_64: {
    return "f64";
  }
  case FloatTypeKind::_80: {
    return "f80";
  }
  case FloatTypeKind::_128: {
    return "f128";
  }
  case FloatTypeKind::_128PPC: {
    return "f128ppc";
  }
  }
}

nuo::Json FloatType::toJson() const {
  return nuo::Json()
      ._("id", getID())
      ._("typeKind", "float")
      ._("floatKind", toString().substr(1));
}

} // namespace qat::IR