#ifndef QAT_AST_NATIVE_TYPE_HPP
#define QAT_AST_NATIVE_TYPE_HPP

#include "../../IR/types/native_type.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class NativeType final : public Type {
	ir::NativeTypeKind nativeKind;

  public:
	NativeType(ir::NativeTypeKind _cTypeKind, FileRange _fileRange) : Type(_fileRange), nativeKind(_cTypeKind) {}

	useit static NativeType* create(ir::NativeTypeKind _cTypeKind, FileRange _fileRange) {
		return std::construct_at(OwnNormal(NativeType), _cTypeKind, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;
	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final { return AstTypeKind::C_TYPE; }

	useit Json   to_json() const final;
	useit String to_string() const final;
};

} // namespace qat::ast

#endif
