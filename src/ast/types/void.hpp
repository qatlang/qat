#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "./qat_type.hpp"

#include <string>

namespace qat::ast {

class VoidType : public QatType {
public:
  VoidType(bool _variable, FileRange _fileRange);

  Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx);
  useit TypeKind     typeKind() const;
  useit Json         toJson() const;
  useit String       toString() const;
};

} // namespace qat::ast

#endif