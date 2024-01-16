#ifndef QAT_AST_TYPES_ARRAY_HPP
#define QAT_AST_TYPES_ARRAY_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class PrerunExpression;

/**
 *  A continuous sequence of elements of a particular type. The sequence
 * is fixed length
 *
 */
class ArrayType : public QatType {
private:
  QatType*                  elementType;
  mutable PrerunExpression* lengthExp;

  void typeInferenceForLength(IR::Context* ctx) const;

public:
  ArrayType(QatType* _element_type, PrerunExpression* _length, FileRange _fileRange)
      : QatType(_fileRange), elementType(_element_type), lengthExp(_length) {}

  useit static inline ArrayType* create(QatType* _element_type, PrerunExpression* _length, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ArrayType), _element_type, _length, _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx);
  useit AstTypeKind  typeKind() const;
  useit Json         toJson() const;
  useit String       toString() const;
};

} // namespace qat::ast

#endif