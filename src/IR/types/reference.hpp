#ifndef QAT_IR_TYPES_REFERENCE_HPP
#define QAT_IR_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"

namespace qat::ir {

class RefType : public Type {
	Type* subType;
	bool  isSubVariable;

  public:
	RefType(bool isSubtypeVariable, Type* _type, ir::Ctx* irCtx);

	useit static RefType* get(bool _isSubtypeVariable, Type* _subtype, ir::Ctx* irCtx);

	useit Type* get_subtype() const;

	useit bool has_variability() const;

	useit bool is_type_sized() const final;

	useit TypeKind type_kind() const final;

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
