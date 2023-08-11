#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class FloatType : public QatType {
private:
  IR::FloatTypeKind kind;

public:
  FloatType(IR::FloatTypeKind _kind, bool _variable, FileRange _fileRange);

  useit static String kindToString(IR::FloatTypeKind kind);
  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx);
  useit TypeKind     typeKind() const;
  useit Json         toJson() const;
  useit String       toString() const;
};

} // namespace qat::ast

#endif