#ifndef QAT_IR_TYPES_FLOAT_HPP
#define QAT_IR_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

namespace qat {
namespace IR {

enum class FloatTypeKind { _brain, _half, _32, _64, _80, _128PPC, _128 };

class FloatType : public QatType {
private:
  FloatTypeKind kind;

public:
  FloatType(const FloatTypeKind _kind);

  FloatTypeKind getKind() const;

  TypeKind typeKind() const;
};
} // namespace IR
} // namespace qat

#endif