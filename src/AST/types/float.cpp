#include "./float.hpp"
namespace qat {
namespace AST {

std::string to_string(FloatTypeKind kind) {
  switch (kind) {
  case FloatTypeKind::_brain: {
    return "brain";
  }
  case FloatTypeKind::_half: {
    return "half";
  }
  case FloatTypeKind::_32: {
    return "32";
  }
  case FloatTypeKind::_64: {
    return "64";
  }
  case FloatTypeKind::_80: {
    return "80";
  }
  case FloatTypeKind::_128PPC: {
    return "128ppc";
  }
  case FloatTypeKind::_128: {
    return "128";
  }
  }
}

FloatType::FloatType(const FloatTypeKind _kind, const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : kind(_kind), QatType(_variable, _filePlacement) {}

llvm::Type *FloatType::emit(IR::Generator *generator) {
  switch (kind) {
  case FloatTypeKind::_brain: {
    return generator->builder.getBFloatTy();
  }
  case FloatTypeKind::_half: {
    return generator->builder.getHalfTy();
  }
  case FloatTypeKind::_32: {
    return generator->builder.getFloatTy();
  }
  case FloatTypeKind::_64: {
    return generator->builder.getDoubleTy();
  }
  case FloatTypeKind::_80: {
    return llvm::Type::getX86_FP80Ty(generator->llvmContext);
  }
  case FloatTypeKind::_128PPC: {
    return llvm::Type::getPPC_FP128Ty(generator->llvmContext);
  }
  case FloatTypeKind::_128: {
    return llvm::Type::getFP128Ty(generator->llvmContext);
  }
  }
}

void FloatType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  std::string value;
  switch (kind) {
  case FloatTypeKind::_brain:
  case FloatTypeKind::_half:
  case FloatTypeKind::_32: {
    value = "float";
    break;
  }
  case FloatTypeKind::_64: {
    value = "double";
    break;
  }
  case FloatTypeKind::_80:
  case FloatTypeKind::_128PPC:
  case FloatTypeKind::_128: {
    value = "long double";
    break;
  }
  }
  if (isConstant()) {
    file += "const ";
  }
  file += value;
  // file.addEnclosedComment(kindToString(kind));
}

std::string FloatType::kindToString(FloatTypeKind kind) {
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

TypeKind FloatType::typeKind() { return TypeKind::Float; }

backend::JSON FloatType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "float")
      ._("floatTypeKind", to_string(kind))
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat