#ifndef QAT_AST_NATIVE_TYPE_HPP
#define QAT_AST_NATIVE_TYPE_HPP

#include "../../IR/types/native_type.hpp"
#include "./address_space.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class NativeType final : public Type {
	ir::NativeTypeKind  nativeKind;
	bool                isNonNullable;
	Maybe<FileRangePtr> varRange;
	Maybe<AddressSpace> addressSpace;

  public:
	NativeType(ir::NativeTypeKind _cTypeKind, bool _isNonNullable, Maybe<FileRangePtr> _varRange,
	           Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Type(_fileRange), nativeKind(_cTypeKind), isNonNullable(_isNonNullable), varRange(_varRange),
	      addressSpace(std::move(_addressSpace)) {}

	useit static NativeType* create_bytestring(bool isNonNullable, Maybe<FileRangePtr> varRange,
	                                           Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType), ir::NativeTypeKind::ByteString, isNonNullable, varRange,
		                         std::move(addressSpace), fileRange);
	}

	useit static NativeType* create_ptrdiff(bool isUnsigned, Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType),
		                         isUnsigned ? ir::NativeTypeKind::UPtrDiff : ir::NativeTypeKind::PtrDiff, false, None,
		                         std::move(addressSpace), fileRange);
	}

	useit static NativeType* create(ir::NativeTypeKind cTypeKind, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType), cTypeKind, false, None, None, fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final;

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final { return AstTypeKind::NATIVE_TYPE; }

	useit String to_string() const final;
};

} // namespace qat::ast

#endif
