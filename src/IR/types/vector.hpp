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

	static Vec<VectorType*> allVectorTypes;

  public:
	VectorType(ir::Type* subType, usize count, VectorKind kind, ir::Ctx* irCtx);
	static VectorType* create(ir::Type* subType, usize count, VectorKind kind, ir::Ctx* irCtx);

	ir::Type* get_element_type() const { return subType; }

	usize get_count() const { return count; }

	bool is_scalable() const { return kind == VectorKind::scalable; }

	bool is_fixed() const { return kind == VectorKind::fixed; }

	ir::VectorType* get_non_scalable_type(ir::Ctx* irCtx) const {
		return VectorType::create(subType, count, VectorKind::fixed, irCtx);
	}

	VectorKind get_vector_kind() const { return kind; }

	TypeKind type_kind() const final { return TypeKind::VECTOR; }

	String to_string() const final;

	bool is_type_sized() const final { return kind == VectorKind::fixed; }

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }
};

} // namespace qat::ir

#endif
