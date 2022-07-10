#include "./float.hpp"

namespace qat {
namespace IR {

FloatType::FloatType(const FloatTypeKind _kind) : kind(_kind) {}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

std::string FloatType::toString() const {
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

} // namespace IR
} // namespace qat