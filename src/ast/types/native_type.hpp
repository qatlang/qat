#ifndef QAT_AST_NATIVE_TYPE_HPP
#define QAT_AST_NATIVE_TYPE_HPP

#include "../../IR/types/native_type.hpp"
#include "./pointer.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class NativeType final : public Type {
	ir::NativeTypeKind  nativeKind;
	Maybe<AddressSpace> addressSpace;

  public:
	NativeType(ir::NativeTypeKind _cTypeKind, Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Type(_fileRange), nativeKind(_cTypeKind), addressSpace(std::move(_addressSpace)) {}

	useit static NativeType* create(ir::NativeTypeKind cTypeKind, Maybe<AddressSpace> addressSpace,
	                                FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType), cTypeKind, std::move(addressSpace), fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*);

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;
	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final { return AstTypeKind::NATIVE_TYPE; }

	useit Json   to_json() const final;
	useit String to_string() const final;
};

} // namespace qat::ast

#endif
