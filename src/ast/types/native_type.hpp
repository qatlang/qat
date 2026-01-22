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

	static NativeType* create_byteptr(bool isNonNullable, Maybe<FileRangePtr> varRange,
	                                  Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType), ir::NativeTypeKind::Byteptr, isNonNullable, varRange,
		                         std::move(addressSpace), fileRange);
	}

	static NativeType* create_ptrdiff(bool isUnsigned, Maybe<AddressSpace> addressSpace, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType),
		                         isUnsigned ? ir::NativeTypeKind::UPtrDiff : ir::NativeTypeKind::PtrDiff, false, None,
		                         std::move(addressSpace), fileRange);
	}

	static NativeType* create(ir::NativeTypeKind cTypeKind, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NativeType), cTypeKind, false, None, None, fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final;

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final { return AstTypeKind::NATIVE_TYPE; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
