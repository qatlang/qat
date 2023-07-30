#ifndef QAT_AST_C_TYPE_HPP
#define QAT_AST_C_TYPE_HPP

#include "../../IR/types/c_type.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class CType : public QatType {
  IR::CTypeKind cTypeKind;

  QatType* subType                  = nullptr;
  bool     isPointerSubTypeVariable = false;

public:
  CType(IR::CTypeKind _cTypeKind, FileRange _fileRange);
  CType(QatType* _pointerSubTy, bool _isPtrSubTyVar, FileRange _fileRange);

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final { return TypeKind::cType; }
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif