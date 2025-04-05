#ifndef QAT_IR_TYPES_SLICE_HPP
#define QAT_IR_TYPES_SLICE_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class SliceType : public Type {
	Type* subType;
	bool  isVar;

  public:
	SliceType(bool isVar, Type* subType, ir::Ctx* ctx);

	useit static SliceType* get(bool isVar, Type* subType, ir::Ctx* ctx);

	useit Type* get_subtype() const { return subType; }

	useit bool has_var() const { return isVar; }

	useit TypeKind type_kind() const { return TypeKind::SLICE; }

	useit String to_string() const;
};

} // namespace qat::ir

#endif
