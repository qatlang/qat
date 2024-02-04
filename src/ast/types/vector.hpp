#ifndef QAT_AST_TYPES_VECTOR_HPP
#define QAT_AST_TYPES_VECTOR_HPP

#include "qat_type.hpp"
#include "type_kind.hpp"
namespace qat::ast {

class VectorType : public QatType {
  QatType*          subType;
  PrerunExpression* count;
  Maybe<FileRange>  scalable;

public:
  VectorType(QatType* _subType, PrerunExpression* _count, Maybe<FileRange> _scalable, FileRange _fileRange)
      : QatType(_fileRange), subType(_subType), count(_count), scalable(_scalable) {}

  useit static inline VectorType* create(QatType* _subType, PrerunExpression* _count, Maybe<FileRange> _scalable,
                                         FileRange _fileRange) {
    return std::construct_at(OwnNormal(VectorType), _subType, _count, _scalable, _fileRange);
  }

  useit IR::QatType* emit(IR::Context* ctx);

  //   useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  AstTypeKind typeKind() const final { return AstTypeKind::VECTOR; }
  Json        toJson() const final;
  String      toString() const final;
};

} // namespace qat::ast

#endif