#ifndef QAT_AST_NATIVE_TYPE_HPP
#define QAT_AST_NATIVE_TYPE_HPP

#include "../../IR/types/native_type.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class NativeType final : public Type {
	ir::NativeTypeKind nativeKind;

	Type* subType				   = nullptr;
	bool  isPointerSubTypeVariable = false;

  public:
	NativeType(ir::NativeTypeKind _cTypeKind, FileRange _fileRange) : Type(_fileRange), nativeKind(_cTypeKind) {}

	NativeType(Type* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange)
		: Type(_fileRange), nativeKind(ir::NativeTypeKind::Pointer), subType(_pointerSubTy),
		  isPointerSubTypeVariable(_isPtrSubTyVar) {}

	useit static NativeType* create(ir::NativeTypeKind _cTypeKind, FileRange _fileRange) {
		return std::construct_at(OwnNormal(NativeType), _cTypeKind, _fileRange);
	}

	useit static NativeType* create(Type* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange) {
		return std::construct_at(OwnNormal(NativeType), _pointerSubTy, _isPtrSubTyVar, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
		if (nativeKind == ir::NativeTypeKind::Pointer) {
			subType->update_dependencies(phase, ir::DependType::partial, ent, ctx);
		}
	}

	useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;
	useit ir::Type*	  emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final { return AstTypeKind::C_TYPE; }
	useit Json		  to_json() const final;
	useit String	  to_string() const final;
};

} // namespace qat::ast

#endif
