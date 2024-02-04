#ifndef QAT_IR_TYPES_VECTOR_HPP
#define QAT_IR_TYPES_VECTOR_HPP

#include "./qat_type.hpp"

namespace qat::IR {

enum class VectorKind {
  fixed,
  scalable,
};

class VectorType : public QatType {
  IR::QatType* subType;
  usize        count;
  VectorKind   kind;

public:
  VectorType(IR::QatType* subType, usize count, VectorKind kind, IR::Context* ctx);
  useit static VectorType* create(IR::QatType* subType, usize count, VectorKind kind, IR::Context* ctx);

  useit inline IR::QatType*    get_element_type() const { return subType; }
  useit inline usize           get_count() const { return count; }
  useit inline bool            is_scalable() const { return kind == VectorKind::scalable; }
  useit inline bool            is_fixed() const { return kind == VectorKind::fixed; }
  useit inline IR::VectorType* get_non_scalable_type(IR::Context* ctx) const {
    return VectorType::create(subType, count, VectorKind::fixed, ctx);
  }
  useit inline VectorKind get_vector_kind() const { return kind; }

  useit TypeKind typeKind() const final { return TypeKind::vector; }
  useit String   toString() const final;

  useit bool isTypeSized() const final { return kind == VectorKind::fixed; }
  useit bool isTriviallyCopyable() const final { return true; }
  useit bool isTriviallyMovable() const final { return true; }
};

} // namespace qat::IR

#endif
