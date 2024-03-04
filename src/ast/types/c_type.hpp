#ifndef QAT_AST_C_TYPE_HPP
#define QAT_AST_C_TYPE_HPP

#include "../../IR/types/c_type.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class CType final : public Type {
  ir::CTypeKind cTypeKind;

  Type* subType                  = nullptr;
  bool  isPointerSubTypeVariable = false;

public:
  CType(ir::CTypeKind _cTypeKind, FileRange _fileRange) : Type(_fileRange), cTypeKind(_cTypeKind) {}

  CType(Type* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange)
      : Type(_fileRange), cTypeKind(ir::CTypeKind::Pointer), subType(_pointerSubTy),
        isPointerSubTypeVariable(_isPtrSubTyVar) {}

  useit static inline CType* create(ir::CTypeKind _cTypeKind, FileRange _fileRange) {
    return std::construct_at(OwnNormal(CType), _cTypeKind, _fileRange);
  }

  useit static inline CType* create(Type* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange) {
    return std::construct_at(OwnNormal(CType), _pointerSubTy, _isPtrSubTyVar, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
    if (cTypeKind == ir::CTypeKind::Pointer) {
      subType->update_dependencies(phase, ir::DependType::partial, ent, ctx);
    }
  }

  useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;
  useit ir::Type*   emit(EmitCtx* ctx) final;
  useit AstTypeKind type_kind() const final { return AstTypeKind::C_TYPE; }
  useit Json        to_json() const final;
  useit String      to_string() const final;
};

} // namespace qat::ast

#endif