#ifndef QAT_IR_TYPES_VECTOR_HPP
#define QAT_IR_TYPES_VECTOR_HPP

#include "./qat_type.hpp"

namespace qat::ir {

enum class VectorKind {
	fixed,
	scalable,
};

class VectorType : public Type {
	ir::Type*  subType;
	usize      count;
	VectorKind kind;

  public:
	VectorType(ir::Type* subType, usize count, VectorKind kind, ir::Ctx* irCtx);
	useit static VectorType* create(ir::Type* subType, usize count, VectorKind kind, ir::Ctx* irCtx);

	useit ir::Type* get_element_type() const { return subType; }

	useit usize get_count() const { return count; }

	useit bool is_scalable() const { return kind == VectorKind::scalable; }

	useit bool is_fixed() const { return kind == VectorKind::fixed; }

	useit ir::VectorType* get_non_scalable_type(ir::Ctx* irCtx) const {
		return VectorType::create(subType, count, VectorKind::fixed, irCtx);
	}

	useit VectorKind get_vector_kind() const { return kind; }

	useit TypeKind type_kind() const final { return TypeKind::VECTOR; }

	useit String to_string() const final;

	useit bool is_type_sized() const final { return kind == VectorKind::fixed; }

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }
};

} // namespace qat::ir

#endif
