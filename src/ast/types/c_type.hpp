#ifndef QAT_AST_C_TYPE_HPP
#define QAT_AST_C_TYPE_HPP

#include "../../IR/types/c_type.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class CType final : public QatType {
  IR::CTypeKind cTypeKind;

  QatType* subType                  = nullptr;
  bool     isPointerSubTypeVariable = false;

public:
  CType(IR::CTypeKind _cTypeKind, FileRange _fileRange) : QatType(_fileRange), cTypeKind(_cTypeKind) {}

  CType(QatType* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange)
      : QatType(_fileRange), cTypeKind(IR::CTypeKind::Pointer), subType(_pointerSubTy),
        isPointerSubTypeVariable(_isPtrSubTyVar) {}

  useit static inline CType* create(IR::CTypeKind _cTypeKind, FileRange _fileRange) {
    return std::construct_at(OwnNormal(CType), _cTypeKind, _fileRange);
  }

  useit static inline CType* create(QatType* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange) {
    return std::construct_at(OwnNormal(CType), _pointerSubTy, _isPtrSubTyVar, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent, IR::Context* ctx) {
    if (cTypeKind == IR::CTypeKind::Pointer) {
      subType->update_dependencies(phase, IR::DependType::partial, ent, ctx);
    }
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final { return AstTypeKind::C_TYPE; }
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif