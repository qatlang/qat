#ifndef QAT_TYPES_REFERENCE_HPP
#define QAT_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include <string>

namespace qat::ast {

class ReferenceType : public QatType {
private:
  QatType* type;
  bool     isSubtypeVar;

public:
  ReferenceType(QatType* _type, bool _isSubtypeVar, FileRange _fileRange)
      : QatType(_fileRange), type(_type), isSubtypeVar(_isSubtypeVar) {}

  useit static inline ReferenceType* create(QatType* _type, bool _isSubtypeVar, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ReferenceType), _type, _isSubtypeVar, _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif