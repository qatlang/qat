#ifndef QAT_IR_TYPES_UNSIGNED_HPP
#define QAT_IR_TYPES_UNSIGNED_HPP

#include "./qat_type.hpp"

namespace qat::IR {

// Unsigned integer datatype in the language
class UnsignedType : public QatType {
private:
  const u32 bitWidth;

public:
  UnsignedType(const u32 _bitWidth);

  TypeKind typeKind() const;

  u64 getBitwidth() const;

  bool isBitWidth(const u32 width) const;

  String toString() const;
};

} // namespace qat::IR

#endif